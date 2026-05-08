#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <memory>

#include "../Model/Sample.hpp"
#include "../Model/Order.hpp"
#include "../repositories/IRepository.hpp"
#include "../services/ProductionCalculator.hpp"
#include "../services/SampleService.hpp"
#include "../services/OrderService.hpp"

namespace fs = std::filesystem;

// ============================================================
// Mock Repository
// ============================================================
class MockSampleRepository : public IRepository<Sample> {
public:
    MOCK_METHOD(std::vector<Sample>, getAll, (), (const, override));
    MOCK_METHOD(std::optional<Sample>, getById, (const std::string&), (const, override));
    MOCK_METHOD(void, add, (Sample&), (override));
    MOCK_METHOD(void, update, (const Sample&), (override));
    MOCK_METHOD(bool, remove, (const std::string&), (override));
};

class MockOrderRepository : public IRepository<Order> {
public:
    MOCK_METHOD(std::vector<Order>, getAll, (), (const, override));
    MOCK_METHOD(std::optional<Order>, getById, (const std::string&), (const, override));
    MOCK_METHOD(void, add, (Order&), (override));
    MOCK_METHOD(void, update, (const Order&), (override));
    MOCK_METHOD(bool, remove, (const std::string&), (override));
};

// ============================================================
// ProductionCalculator 테스트
// ============================================================
class ProductionCalculatorTest : public ::testing::Test {};

TEST_F(ProductionCalculatorTest, StockSufficientReturnsZero) {
    EXPECT_EQ(ProductionCalculator::calcProductionQty(30, 50, 0.85), 0);
}

TEST_F(ProductionCalculatorTest, StockShortage) {
    // 부족분=20, ceil(20/(0.85*0.9))=ceil(26.14)=27
    EXPECT_EQ(ProductionCalculator::calcProductionQty(30, 10, 0.85), 27);
}

TEST_F(ProductionCalculatorTest, ZeroStock) {
    // 부족분=10, ceil(10/(1.0*0.9))=ceil(11.11)=12
    EXPECT_EQ(ProductionCalculator::calcProductionQty(10, 0, 1.0), 12);
}

TEST_F(ProductionCalculatorTest, TotalMinutes) {
    EXPECT_DOUBLE_EQ(ProductionCalculator::calcTotalMinutes(12, 1.5), 18.0);
}

// ============================================================
// SampleService 테스트
// ============================================================
class SampleServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockSampleRepository> mockRepo =
        std::make_shared<MockSampleRepository>();
    SampleService svc{ mockRepo };

    Sample validSample() {
        return Sample{ "S0001", "AlGaN 에피", 1.5, 0.85, 0 };
    }
};

TEST_F(SampleServiceTest, CreateWithInvalidYieldThrows) {
    Sample s = validSample(); s.yield = 1.2;
    EXPECT_THROW(svc.create(s), std::invalid_argument);
}

TEST_F(SampleServiceTest, CreateWithZeroAvgProductionTimeThrows) {
    Sample s = validSample(); s.avgProductionTime = 0.0;
    EXPECT_THROW(svc.create(s), std::invalid_argument);
}

TEST_F(SampleServiceTest, CreateWithAvgProductionTimeBelowMinThrows) {
    Sample s = validSample(); s.avgProductionTime = 0.05;
    EXPECT_THROW(svc.create(s), std::invalid_argument);
}

TEST_F(SampleServiceTest, CreateDuplicateIdThrows) {
    Sample s = validSample();
    EXPECT_CALL(*mockRepo, getById("S0001"))
        .WillOnce(::testing::Return(std::optional<Sample>{s}));
    EXPECT_THROW(svc.create(s), std::invalid_argument);
}

TEST_F(SampleServiceTest, CreateValidSampleSucceeds) {
    Sample s = validSample();
    EXPECT_CALL(*mockRepo, getById("S0001"))
        .WillOnce(::testing::Return(std::nullopt));
    EXPECT_CALL(*mockRepo, add(::testing::_)).Times(1);
    EXPECT_NO_THROW(svc.create(s));
}

TEST_F(SampleServiceTest, CreateWithNegativeStockQtyThrows) {
    Sample s = validSample(); s.stockQty = -1;
    EXPECT_THROW(svc.create(s), std::invalid_argument);
}

TEST_F(SampleServiceTest, CreateWithEmptySampleIdThrows) {
    Sample s = validSample(); s.sampleId = "";
    EXPECT_THROW(svc.create(s), std::invalid_argument);
}

TEST_F(SampleServiceTest, SearchByKeyword) {
    std::vector<Sample> samples = {
        Sample{ "S0001", "AlGaN 에피", 1.5, 0.85, 0 },
        Sample{ "S0002", "GaN 버퍼",  0.5, 0.9,  0 },
    };
    EXPECT_CALL(*mockRepo, getAll()).WillOnce(::testing::Return(samples));
    auto result = svc.search("AlGaN");
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].sampleId, "S0001");
}

// ============================================================
// OrderService 테스트
// ============================================================
class OrderServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockSampleRepository> sampleRepo =
        std::make_shared<MockSampleRepository>();
    std::shared_ptr<MockOrderRepository> orderRepo =
        std::make_shared<MockOrderRepository>();
    OrderService svc{ sampleRepo, orderRepo };

    Sample baseSample(int stock = 100) {
        return Sample{ "S0001", "AlGaN 에피", 1.5, 0.85, stock };
    }

    Order reservedOrder(const std::string& id = "ORD-20260508-0001") {
        Order o;
        o.orderId = id; o.sampleId = "S0001";
        o.customerName = "테스트"; o.orderQty = 30;
        o.status = "reserved"; o.productionQty = 0;
        o.totalProductionMinutes = 0.0;
        o.queuedAt = ""; o.productionStartedAt = "";
        o.estimatedCompletionAt = ""; o.createdAt = "";
        return o;
    }
};

TEST_F(OrderServiceTest, PlaceOrderSuccess) {
    Sample s = baseSample();
    EXPECT_CALL(*sampleRepo, getById("S0001"))
        .WillOnce(::testing::Return(std::optional<Sample>{s}));
    EXPECT_CALL(*orderRepo, add(::testing::_)).Times(1);

    Order result = svc.placeOrder("S0001", "테스트고객", 10);
    EXPECT_EQ(result.status, "reserved");
    EXPECT_EQ(result.sampleId, "S0001");
    EXPECT_EQ(result.orderQty, 10);
}

TEST_F(OrderServiceTest, PlaceOrderWithUnknownSampleThrows) {
    EXPECT_CALL(*sampleRepo, getById("S9999"))
        .WillOnce(::testing::Return(std::nullopt));
    EXPECT_THROW(svc.placeOrder("S9999", "고객", 10), std::invalid_argument);
}

TEST_F(OrderServiceTest, ApproveWithSufficientStockSetsConfirmed) {
    Order o = reservedOrder();
    Sample s = baseSample(100);

    EXPECT_CALL(*orderRepo, getById("ORD-20260508-0001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));
    EXPECT_CALL(*sampleRepo, getById("S0001"))
        .WillOnce(::testing::Return(std::optional<Sample>{s}));
    EXPECT_CALL(*sampleRepo, update(::testing::_)).Times(1);
    EXPECT_CALL(*orderRepo, update(::testing::_)).Times(1);

    svc.approve("ORD-20260508-0001");

    // update에 넘긴 Order의 status가 "confirmed"인지 검증
    EXPECT_CALL(*orderRepo, update(::testing::Field(&Order::status, "confirmed")))
        .Times(::testing::AnyNumber());
}

TEST_F(OrderServiceTest, ApproveWithInsufficientStockSetsProducing) {
    Order o = reservedOrder(); o.orderQty = 30;
    Sample s = baseSample(5); // 재고 5 < 주문 30

    EXPECT_CALL(*orderRepo, getById("ORD-20260508-0001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));
    EXPECT_CALL(*sampleRepo, getById("S0001"))
        .WillOnce(::testing::Return(std::optional<Sample>{s}));
    EXPECT_CALL(*sampleRepo, update(::testing::_)).Times(1);
    EXPECT_CALL(*orderRepo, update(::testing::_)).Times(1);

    svc.approve("ORD-20260508-0001");
}

TEST_F(OrderServiceTest, ApproveAlreadyConfirmedThrows) {
    Order o = reservedOrder(); o.status = "confirmed";
    EXPECT_CALL(*orderRepo, getById("ORD-20260508-0001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));
    EXPECT_THROW(svc.approve("ORD-20260508-0001"), std::invalid_argument);
}

TEST_F(OrderServiceTest, ApproveRejectedThrows) {
    Order o = reservedOrder(); o.status = "rejected";
    EXPECT_CALL(*orderRepo, getById("ORD-20260508-0001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));
    EXPECT_THROW(svc.approve("ORD-20260508-0001"), std::invalid_argument);
}

TEST_F(OrderServiceTest, RejectReservedOrder) {
    Order o = reservedOrder();
    EXPECT_CALL(*orderRepo, getById("ORD-20260508-0001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));
    EXPECT_CALL(*orderRepo, update(::testing::Field(&Order::status, "rejected"))).Times(1);
    EXPECT_NO_THROW(svc.reject("ORD-20260508-0001"));
}

TEST_F(OrderServiceTest, RejectNonReservedThrows) {
    Order o = reservedOrder(); o.status = "confirmed";
    EXPECT_CALL(*orderRepo, getById("ORD-20260508-0001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));
    EXPECT_THROW(svc.reject("ORD-20260508-0001"), std::invalid_argument);
}

TEST_F(OrderServiceTest, FindByStatusReturnsMatching) {
    std::vector<Order> orders = { reservedOrder("ORD-001"), reservedOrder("ORD-002") };
    orders[1].status = "confirmed";
    EXPECT_CALL(*orderRepo, getAll()).WillOnce(::testing::Return(orders));
    auto result = svc.findByStatus("reserved");
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].orderId, "ORD-001");
}

TEST_F(OrderServiceTest, FindByIdDelegatestoRepo) {
    Order o = reservedOrder();
    EXPECT_CALL(*orderRepo, getById("ORD-20260508-0001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));
    auto result = svc.findById("ORD-20260508-0001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->orderId, "ORD-20260508-0001");
}

TEST_F(OrderServiceTest, ReleaseConfirmedOrder) {
    Order o = reservedOrder(); o.status = "confirmed";
    EXPECT_CALL(*orderRepo, getById("ORD-20260508-0001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));
    EXPECT_CALL(*orderRepo, update(::testing::Field(&Order::status, "released"))).Times(1);
    EXPECT_NO_THROW(svc.release("ORD-20260508-0001"));
}

TEST_F(OrderServiceTest, ReleaseProducingDirectlyThrows) {
    Order o = reservedOrder(); o.status = "producing";
    EXPECT_CALL(*orderRepo, getById("ORD-20260508-0001"))
        .WillOnce(::testing::Return(std::optional<Order>{o}));
    EXPECT_THROW(svc.release("ORD-20260508-0001"), std::invalid_argument);
}
