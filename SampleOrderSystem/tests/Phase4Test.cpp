#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <chrono>
#include <algorithm>

#include "../clock/MockClock.h"
#include "../Model/Order.hpp"
#include "../Model/Sample.hpp"
#include "../repositories/IRepository.hpp"
#include "../services/ProductionService.hpp"
#include "../Utils/TimeUtils.hpp"

// ============================================================
// Mock Repository
// ============================================================
class MockOrderRepo2 : public IRepository<Order> {
public:
    MOCK_METHOD(std::vector<Order>, getAll, (), (const, override));
    MOCK_METHOD(std::optional<Order>, getById, (const std::string&), (const, override));
    MOCK_METHOD(void, add, (Order&), (override));
    MOCK_METHOD(void, update, (const Order&), (override));
    MOCK_METHOD(bool, remove, (const std::string&), (override));
};

class MockSampleRepo2 : public IRepository<Sample> {
public:
    MOCK_METHOD(std::vector<Sample>, getAll, (), (const, override));
    MOCK_METHOD(std::optional<Sample>, getById, (const std::string&), (const, override));
    MOCK_METHOD(void, add, (Sample&), (override));
    MOCK_METHOD(void, update, (const Sample&), (override));
    MOCK_METHOD(bool, remove, (const std::string&), (override));
};

// ============================================================
// 헬퍼
// ============================================================
using TP = std::chrono::system_clock::time_point;

// now = 2026-05-08 10:00:00 UTC 기준
static const TP BASE = std::chrono::system_clock::from_time_t(1746694800);

static Order makeActive(const std::string& id, const std::string& sampleId,
                        TP startTp, TP endTp, double totalMin) {
    Order o;
    o.orderId = id; o.sampleId = sampleId;
    o.customerName = "고객"; o.orderQty = 10;
    o.status = "producing";
    o.productionQty = 12; o.totalProductionMinutes = totalMin;
    o.queuedAt              = TimeUtils::toIso8601(startTp);
    o.productionStartedAt   = TimeUtils::toIso8601(startTp);
    o.estimatedCompletionAt = TimeUtils::toIso8601(endTp);
    o.createdAt = "2026-05-08T00:00:00";
    return o;
}

static Order makeQueued(const std::string& id, const std::string& sampleId,
                        TP queuedTp, double totalMin) {
    Order o;
    o.orderId = id; o.sampleId = sampleId;
    o.customerName = "고객"; o.orderQty = 10;
    o.status = "producing";
    o.productionQty = 12; o.totalProductionMinutes = totalMin;
    o.queuedAt = TimeUtils::toIso8601(queuedTp);
    o.productionStartedAt   = "";
    o.estimatedCompletionAt = "";
    o.createdAt = "2026-05-08T00:00:00";
    return o;
}

// ============================================================
// 테스트 픽스처
// ============================================================
class ProductionServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockClock>       clock      = std::make_shared<MockClock>(BASE);
    std::shared_ptr<MockOrderRepo2>  orderRepo  = std::make_shared<MockOrderRepo2>();
    std::shared_ptr<MockSampleRepo2> sampleRepo = std::make_shared<MockSampleRepo2>();
    ProductionService svc{ clock, orderRepo, sampleRepo };

    // 상태를 반영하는 stateful mock 설정
    // orders 벡터를 getAll/update 람다가 공유하므로 update 후 getAll이 갱신된 값을 반환함
    void setupStatefulMock(std::vector<Order>& orders) {
        EXPECT_CALL(*orderRepo, getAll())
            .WillRepeatedly([&orders]() { return orders; });
        EXPECT_CALL(*orderRepo, update(::testing::_))
            .WillRepeatedly([&orders](const Order& o) {
                for (auto& e : orders)
                    if (e.orderId == o.orderId) e = o;
            });
        EXPECT_CALL(*sampleRepo, getById(::testing::_))
            .WillRepeatedly(::testing::Return(std::nullopt));
        EXPECT_CALL(*sampleRepo, update(::testing::_))
            .WillRepeatedly(::testing::Return());
    }
};

// ── 케이스 1: 완료 시각이 미래 → 상태 변화 없음 ──────────────
TEST_F(ProductionServiceTest, FutureCompletionNoChange) {
    Order o = makeActive("ORD-A", "S0001",
                         BASE - std::chrono::minutes(30),
                         BASE + std::chrono::minutes(60), 90.0);
    std::vector<Order> orders = {o};
    setupStatefulMock(orders);

    svc.applyLazyUpdates();

    EXPECT_EQ(orders[0].status, "producing");
}

// ── 케이스 2: 완료 시각 경과 → confirmed ─────────────────────
TEST_F(ProductionServiceTest, CompletedOrderBecomesConfirmed) {
    Order o = makeActive("ORD-A", "S0001",
                         BASE - std::chrono::minutes(70),
                         BASE - std::chrono::minutes(10), 60.0);
    std::vector<Order> orders = {o};
    setupStatefulMock(orders);

    svc.applyLazyUpdates();

    EXPECT_EQ(orders[0].status, "confirmed");
}

// ── 케이스 3: A 완료, 대기 2개(B·C) → queuedAt 앞선 B 먼저 시작 ─
TEST_F(ProductionServiceTest, CompletedOrderStartsEarliestQueuedFirst) {
    Order A = makeActive("ORD-A", "S0001",
                         BASE - std::chrono::minutes(65),
                         BASE - std::chrono::minutes(5), 60.0);
    // B가 나중에 큐 진입, C가 먼저 진입
    Order B = makeQueued("ORD-B", "S0001", BASE - std::chrono::minutes(40), 20.0);
    Order C = makeQueued("ORD-C", "S0001", BASE - std::chrono::minutes(50), 20.0);
    std::vector<Order> orders = {A, B, C};
    setupStatefulMock(orders);

    svc.applyLazyUpdates();

    // C가 먼저 큐 진입했으므로 C부터 생산 시작
    auto& cFinal = orders[2];
    EXPECT_FALSE(cFinal.productionStartedAt.empty());
    EXPECT_EQ(cFinal.status, "producing");
}

// ── 케이스 3b: A 완료 → B 생산 시작 ─────────────────────────
TEST_F(ProductionServiceTest, CompletedOrderStartsNextInQueue) {
    Order A = makeActive("ORD-A", "S0001",
                         BASE - std::chrono::minutes(65),
                         BASE - std::chrono::minutes(5), 60.0);
    Order B = makeQueued("ORD-B", "S0001",
                         BASE - std::chrono::minutes(60), 30.0);
    std::vector<Order> orders = {A, B};
    setupStatefulMock(orders);

    svc.applyLazyUpdates();

    auto& aFinal = orders[0]; auto& bFinal = orders[1];
    EXPECT_EQ(aFinal.status, "confirmed");
    EXPECT_FALSE(bFinal.productionStartedAt.empty());
    EXPECT_EQ(bFinal.status, "producing");
}

// ── 케이스 4: A·B 모두 완료 시간 경과 → 연쇄 confirmed ──────
TEST_F(ProductionServiceTest, BothOrdersCompletedCascade) {
    // A: base-125min 시작, 60분 소요 → base-65min 완료 (과거)
    // B: A 완료 후 시작 → base-65min, 30분 소요 → base-35min 완료 (과거)
    Order A = makeActive("ORD-A", "S0001",
                         BASE - std::chrono::minutes(125),
                         BASE - std::chrono::minutes(65), 60.0);
    Order B = makeQueued("ORD-B", "S0001",
                         BASE - std::chrono::minutes(120), 30.0);
    std::vector<Order> orders = {A, B};
    setupStatefulMock(orders);

    svc.applyLazyUpdates();

    EXPECT_EQ(orders[0].status, "confirmed");
    EXPECT_EQ(orders[1].status, "confirmed");
}

// ── 케이스 5: B의 productionStartedAt >= A의 estimatedCompletionAt ──
TEST_F(ProductionServiceTest, NextOrderStartsAfterPrevCompletion) {
    std::string aEndStr = TimeUtils::toIso8601(BASE - std::chrono::minutes(10));
    Order A = makeActive("ORD-A", "S0001",
                         BASE - std::chrono::minutes(70),
                         BASE - std::chrono::minutes(10), 60.0);
    Order B = makeQueued("ORD-B", "S0001",
                         BASE - std::chrono::minutes(60), 30.0);
    std::vector<Order> orders = {A, B};
    setupStatefulMock(orders);

    svc.applyLazyUpdates();

    EXPECT_GE(orders[1].productionStartedAt, aEndStr);
}

// ── 케이스 6: getActiveOrder() — 없을 때 nullopt ────────────
TEST_F(ProductionServiceTest, GetActiveOrderReturnsNulloptWhenNone) {
    EXPECT_CALL(*orderRepo, getAll())
        .WillOnce(::testing::Return(std::vector<Order>{}));

    EXPECT_FALSE(svc.getActiveOrder().has_value());
}

// ── 케이스 6b: getActiveOrder() — 활성 주문 반환 ─────────────
TEST_F(ProductionServiceTest, GetActiveOrderReturnsActiveOrder) {
    Order A = makeActive("ORD-A", "S0001",
                         BASE - std::chrono::minutes(30),
                         BASE + std::chrono::minutes(30), 60.0);
    EXPECT_CALL(*orderRepo, getAll())
        .WillOnce(::testing::Return(std::vector<Order>{A}));

    auto result = svc.getActiveOrder();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->orderId, "ORD-A");
}

// ── 케이스 7: getQueue() — queuedAt 오름차순 ────────────────
TEST_F(ProductionServiceTest, GetQueueReturnsSortedByQueuedAt) {
    Order B = makeQueued("ORD-B", "S0001", BASE - std::chrono::minutes(30), 30.0);
    Order C = makeQueued("ORD-C", "S0001", BASE - std::chrono::minutes(60), 20.0);

    EXPECT_CALL(*orderRepo, getAll())
        .WillOnce(::testing::Return(std::vector<Order>{B, C}));

    auto queue = svc.getQueue();
    ASSERT_EQ(queue.size(), 2u);
    EXPECT_EQ(queue[0].orderId, "ORD-C"); // 더 먼저 진입
    EXPECT_EQ(queue[1].orderId, "ORD-B");
}

// ── 케이스 8: getRemainingMinutes — 정상 ─────────────────────
TEST_F(ProductionServiceTest, GetRemainingMinutes) {
    Order o = makeActive("ORD-001", "S0001",
                         BASE - std::chrono::minutes(30),
                         BASE + std::chrono::minutes(30), 60.0);

    EXPECT_CALL(*orderRepo, getById("ORD-001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));

    EXPECT_NEAR(svc.getRemainingMinutes("ORD-001"), 30.0, 1.0);
}

// ── 케이스 8b: getRemainingMinutes — 음수이면 0 ──────────────
TEST_F(ProductionServiceTest, GetRemainingMinutesNegativeReturnsZero) {
    Order o = makeActive("ORD-001", "S0001",
                         BASE - std::chrono::minutes(70),
                         BASE - std::chrono::minutes(10), 60.0);

    EXPECT_CALL(*orderRepo, getById("ORD-001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));

    EXPECT_EQ(svc.getRemainingMinutes("ORD-001"), 0.0);
}
