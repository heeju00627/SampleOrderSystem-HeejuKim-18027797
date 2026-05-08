#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <chrono>

#include "../clock/MockClock.h"
#include "../Model/Order.hpp"
#include "../Model/Sample.hpp"
#include "../repositories/IRepository.hpp"
#include "../services/ProductionService.hpp"

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
// 헬퍼: 주문·시료 생성
// ============================================================
using TP = std::chrono::system_clock::time_point;

static Order makeProducingOrder(const std::string& id,
                                const std::string& sampleId,
                                const std::string& startedAt,
                                const std::string& completionAt,
                                double totalMinutes,
                                const std::string& queuedAt = "")
{
    Order o;
    o.orderId = id; o.sampleId = sampleId;
    o.customerName = "고객"; o.orderQty = 10;
    o.status = "producing";
    o.productionQty = 12; o.totalProductionMinutes = totalMinutes;
    o.queuedAt = queuedAt.empty() ? startedAt : queuedAt;
    o.productionStartedAt = startedAt;
    o.estimatedCompletionAt = completionAt;
    o.createdAt = "2026-05-08T00:00:00";
    return o;
}

static Order makeQueuedOrder(const std::string& id,
                             const std::string& sampleId,
                             const std::string& queuedAt,
                             double totalMinutes)
{
    Order o;
    o.orderId = id; o.sampleId = sampleId;
    o.customerName = "고객"; o.orderQty = 10;
    o.status = "producing";
    o.productionQty = 12; o.totalProductionMinutes = totalMinutes;
    o.queuedAt = queuedAt;
    o.productionStartedAt = "";   // 대기 중
    o.estimatedCompletionAt = "";
    o.createdAt = "2026-05-08T00:00:00";
    return o;
}

static Sample makeSample(const std::string& id = "S0001") {
    return Sample{ id, "AlGaN", 1.5, 0.85, 0 };
}

// ============================================================
// ProductionService 테스트 픽스처
// ============================================================
class ProductionServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockClock>      clock     = std::make_shared<MockClock>();
    std::shared_ptr<MockOrderRepo2> orderRepo = std::make_shared<MockOrderRepo2>();
    std::shared_ptr<MockSampleRepo2> sampleRepo = std::make_shared<MockSampleRepo2>();
    ProductionService svc{ clock, orderRepo, sampleRepo };

    // now = 2026-05-08 10:00:00 기준
    TP base = std::chrono::system_clock::from_time_t(1746694800);
};

// ── 케이스 1: estimatedCompletionAt이 미래 → 변화 없음 ──────
TEST_F(ProductionServiceTest, FutureCompletionNoChange) {
    clock->setNow(base);
    // completionAt = base + 60분 (미래)
    std::string futureAt = TimeUtils::toIso8601(base + std::chrono::minutes(60));
    std::string startAt  = TimeUtils::toIso8601(base - std::chrono::minutes(30));
    Order o = makeProducingOrder("ORD-001", "S0001", startAt, futureAt, 90.0);

    EXPECT_CALL(*orderRepo, getAll())
        .WillRepeatedly(::testing::Return(std::vector<Order>{o}));
    EXPECT_CALL(*orderRepo, update(::testing::_)).Times(0);

    svc.applyLazyUpdates();
}

// ── 케이스 2: 완료 시각 경과 → confirmed ────────────────────
TEST_F(ProductionServiceTest, CompletedOrderBecomesConfirmed) {
    clock->setNow(base);
    std::string pastAt  = TimeUtils::toIso8601(base - std::chrono::minutes(10));
    std::string startAt = TimeUtils::toIso8601(base - std::chrono::minutes(70));
    Order o = makeProducingOrder("ORD-001", "S0001", startAt, pastAt, 60.0);

    Order captured;
    EXPECT_CALL(*orderRepo, getAll())
        .WillRepeatedly(::testing::Return(std::vector<Order>{o}));
    EXPECT_CALL(*orderRepo, update(::testing::_))
        .WillOnce(::testing::SaveArg<0>(&captured));
    EXPECT_CALL(*sampleRepo, getById(::testing::_)).Times(::testing::AnyNumber());
    EXPECT_CALL(*sampleRepo, update(::testing::_)).Times(::testing::AnyNumber());

    svc.applyLazyUpdates();
    EXPECT_EQ(captured.status, "confirmed");
}

// ── 케이스 3: A 완료 → B 생산 시작 ─────────────────────────
TEST_F(ProductionServiceTest, CompletedOrderStartsNextInQueue) {
    clock->setNow(base);
    std::string pastAt   = TimeUtils::toIso8601(base - std::chrono::minutes(5));
    std::string startAt  = TimeUtils::toIso8601(base - std::chrono::minutes(65));
    std::string queuedAt = TimeUtils::toIso8601(base - std::chrono::minutes(60));
    Order A = makeProducingOrder("ORD-A", "S0001", startAt, pastAt, 60.0);
    Order B = makeQueuedOrder("ORD-B", "S0001", queuedAt, 30.0);

    std::vector<Order> captured;
    EXPECT_CALL(*orderRepo, getAll())
        .WillRepeatedly(::testing::Return(std::vector<Order>{A, B}));
    EXPECT_CALL(*orderRepo, update(::testing::_))
        .WillRepeatedly([&](const Order& o) { captured.push_back(o); });
    EXPECT_CALL(*sampleRepo, getById(::testing::_)).Times(::testing::AnyNumber());
    EXPECT_CALL(*sampleRepo, update(::testing::_)).Times(::testing::AnyNumber());

    svc.applyLazyUpdates();

    ASSERT_GE(captured.size(), 2u);
    auto aIt = std::find_if(captured.begin(), captured.end(),
        [](const Order& o) { return o.orderId == "ORD-A"; });
    auto bIt = std::find_if(captured.begin(), captured.end(),
        [](const Order& o) { return o.orderId == "ORD-B"; });
    ASSERT_NE(aIt, captured.end());
    ASSERT_NE(bIt, captured.end());
    EXPECT_EQ(aIt->status, "confirmed");
    EXPECT_FALSE(bIt->productionStartedAt.empty());
}

// ── 케이스 4: A·B 모두 완료 시간 경과 → 연쇄 confirmed ──────
TEST_F(ProductionServiceTest, BothOrdersCompletedCascade) {
    clock->setNow(base);
    // A: 완료됨 (base - 125분 시작, 60분 소요 → base - 65분에 완료)
    std::string aStart = TimeUtils::toIso8601(base - std::chrono::minutes(125));
    std::string aEnd   = TimeUtils::toIso8601(base - std::chrono::minutes(65));
    // B: 대기 (A 완료 후 시작 → aEnd, 30분 소요 → base - 35분에 완료)
    std::string bQueued = TimeUtils::toIso8601(base - std::chrono::minutes(120));
    Order A = makeProducingOrder("ORD-A", "S0001", aStart, aEnd, 60.0);
    Order B = makeQueuedOrder("ORD-B", "S0001", bQueued, 30.0);

    std::vector<Order> all = {A, B};
    EXPECT_CALL(*orderRepo, getAll())
        .WillRepeatedly([&]() { return all; });
    EXPECT_CALL(*orderRepo, update(::testing::_))
        .WillRepeatedly([&](const Order& o) {
            for (auto& existing : all)
                if (existing.orderId == o.orderId) existing = o;
        });
    EXPECT_CALL(*sampleRepo, getById(::testing::_)).Times(::testing::AnyNumber());
    EXPECT_CALL(*sampleRepo, update(::testing::_)).Times(::testing::AnyNumber());

    svc.applyLazyUpdates();

    auto aFinal = std::find_if(all.begin(), all.end(),
        [](const Order& o){ return o.orderId == "ORD-A"; });
    auto bFinal = std::find_if(all.begin(), all.end(),
        [](const Order& o){ return o.orderId == "ORD-B"; });
    EXPECT_EQ(aFinal->status, "confirmed");
    EXPECT_EQ(bFinal->status, "confirmed");
}

// ── 케이스 5: B의 startedAt >= A의 completionAt ──────────────
TEST_F(ProductionServiceTest, NextOrderStartsAfterPrevCompletion) {
    clock->setNow(base);
    std::string aStart = TimeUtils::toIso8601(base - std::chrono::minutes(70));
    std::string aEnd   = TimeUtils::toIso8601(base - std::chrono::minutes(10));
    std::string bQueue = TimeUtils::toIso8601(base - std::chrono::minutes(60));
    Order A = makeProducingOrder("ORD-A", "S0001", aStart, aEnd, 60.0);
    Order B = makeQueuedOrder("ORD-B", "S0001", bQueue, 30.0);

    std::vector<Order> all = {A, B};
    EXPECT_CALL(*orderRepo, getAll()).WillRepeatedly([&]() { return all; });
    EXPECT_CALL(*orderRepo, update(::testing::_))
        .WillRepeatedly([&](const Order& o) {
            for (auto& e : all) if (e.orderId == o.orderId) e = o;
        });
    EXPECT_CALL(*sampleRepo, getById(::testing::_)).Times(::testing::AnyNumber());
    EXPECT_CALL(*sampleRepo, update(::testing::_)).Times(::testing::AnyNumber());

    svc.applyLazyUpdates();

    auto bFinal = std::find_if(all.begin(), all.end(),
        [](const Order& o){ return o.orderId == "ORD-B"; });
    // B의 생산 시작 시각 >= A의 완료 시각
    EXPECT_GE(bFinal->productionStartedAt, aEnd);
}

// ── 케이스 6: getActiveOrder() — 없을 때 nullopt ────────────
TEST_F(ProductionServiceTest, GetActiveOrderReturnsNulloptWhenNone) {
    EXPECT_CALL(*orderRepo, getAll())
        .WillOnce(::testing::Return(std::vector<Order>{}));
    EXPECT_FALSE(svc.getActiveOrder().has_value());
}

// ── 케이스 7: getQueue() — queuedAt 오름차순 ────────────────
TEST_F(ProductionServiceTest, GetQueueReturnsSortedByQueuedAt) {
    clock->setNow(base);
    std::string q1 = TimeUtils::toIso8601(base - std::chrono::minutes(30));
    std::string q2 = TimeUtils::toIso8601(base - std::chrono::minutes(60));
    Order B = makeQueuedOrder("ORD-B", "S0001", q1, 30.0);
    Order C = makeQueuedOrder("ORD-C", "S0001", q2, 20.0);

    EXPECT_CALL(*orderRepo, getAll())
        .WillOnce(::testing::Return(std::vector<Order>{B, C}));

    auto queue = svc.getQueue();
    ASSERT_EQ(queue.size(), 2u);
    EXPECT_EQ(queue[0].orderId, "ORD-C"); // 더 먼저 진입
    EXPECT_EQ(queue[1].orderId, "ORD-B");
}

// ── 케이스 8: getRemainingMinutes — 정상 및 음수 방어 ────────
TEST_F(ProductionServiceTest, GetRemainingMinutes) {
    clock->setNow(base);
    // 잔여 = 완료 시각 - 현재 = base+30분 - base = 30분
    std::string endAt   = TimeUtils::toIso8601(base + std::chrono::minutes(30));
    std::string startAt = TimeUtils::toIso8601(base - std::chrono::minutes(30));
    Order o = makeProducingOrder("ORD-001", "S0001", startAt, endAt, 60.0);

    EXPECT_CALL(*orderRepo, getById("ORD-001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));

    double remaining = svc.getRemainingMinutes("ORD-001");
    EXPECT_NEAR(remaining, 30.0, 1.0);
}

TEST_F(ProductionServiceTest, GetRemainingMinutesNegativeReturnsZero) {
    clock->setNow(base);
    // completionAt이 이미 지난 경우 → 0 반환
    std::string pastAt  = TimeUtils::toIso8601(base - std::chrono::minutes(10));
    std::string startAt = TimeUtils::toIso8601(base - std::chrono::minutes(70));
    Order o = makeProducingOrder("ORD-001", "S0001", startAt, pastAt, 60.0);

    EXPECT_CALL(*orderRepo, getById("ORD-001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));

    double remaining = svc.getRemainingMinutes("ORD-001");
    EXPECT_EQ(remaining, 0.0);
}
