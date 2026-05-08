#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

#include "../Model/Sample.hpp"
#include "../Model/Order.hpp"
#include "../repositories/SampleRepository.hpp"
#include "../repositories/OrderRepository.hpp"

namespace fs = std::filesystem;

// ────────────────────────────────────────────────
// 헬퍼: 테스트 전후 임시 파일 정리
// ────────────────────────────────────────────────
struct TempFile {
    std::string path;
    explicit TempFile(const std::string& p) : path(p) { fs::remove(path); }
    ~TempFile() { fs::remove(path); }
};

// ============================================================
// Sample 직렬화 테스트
// ============================================================
class SampleSerializationTest : public ::testing::Test {};

TEST_F(SampleSerializationTest, RoundTrip) {
    Sample s{ "S0001", "AlGaN 에피", 1.5, 0.85, 50 };
    Sample result = sampleFromJson(sampleToJson(s));
    EXPECT_EQ(result.sampleId, s.sampleId);
    EXPECT_EQ(result.name, s.name);
    EXPECT_DOUBLE_EQ(result.avgProductionTime, s.avgProductionTime);
    EXPECT_DOUBLE_EQ(result.yield, s.yield);
    EXPECT_EQ(result.stockQty, s.stockQty);
}

TEST_F(SampleSerializationTest, MissingStockQtyDefaultsToZero) {
    auto obj = json::makeObject();
    obj["sampleId"]          = std::string("S0002");
    obj["name"]              = std::string("GaN 버퍼");
    obj["avgProductionTime"] = 0.5;
    obj["yield"]             = 0.9;
    // stockQty 없음

    Sample s = sampleFromJson(obj);
    EXPECT_EQ(s.stockQty, 0);
}

// ============================================================
// SampleRepository CRUD 테스트
// ============================================================
class SampleRepositoryTest : public ::testing::Test {
protected:
    std::string filePath = "test_samples_temp.json";
    TempFile    tempFile{ filePath };

    std::unique_ptr<JsonRepository<Sample>> repo() {
        return makeSampleRepository(filePath);
    }
};

TEST_F(SampleRepositoryTest, GetAllOnMissingFileReturnsEmpty) {
    auto r = repo();
    EXPECT_TRUE(r->getAll().empty());
}

TEST_F(SampleRepositoryTest, FindByIdReturnsCorrectSample) {
    auto r = repo();
    Sample s{ "S0001", "AlGaN 에피", 1.5, 0.85, 10 };
    r->add(s);

    auto found = r->getById("S0001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->sampleId, "S0001");
    EXPECT_DOUBLE_EQ(found->avgProductionTime, 1.5);
}

TEST_F(SampleRepositoryTest, FindByIdReturnsNulloptForUnknownId) {
    auto r = repo();
    Sample s{ "S0001", "AlGaN 에피", 1.5, 0.85, 10 };
    r->add(s);

    auto found = r->getById("S9999");
    EXPECT_FALSE(found.has_value());
}

TEST_F(SampleRepositoryTest, UpdatePersistsChange) {
    auto r = repo();
    Sample s{ "S0001", "AlGaN 에피", 1.5, 0.85, 10 };
    r->add(s);

    s.stockQty = 99;
    r->update(s);

    auto found = r->getById("S0001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->stockQty, 99);
}

// ============================================================
// Order 직렬화 테스트
// ============================================================
class OrderSerializationTest : public ::testing::Test {};

TEST_F(OrderSerializationTest, RoundTrip) {
    Order o;
    o.orderId               = "ORD-20260508-0001";
    o.sampleId              = "S0001";
    o.customerName          = "한국반도체";
    o.orderQty              = 30;
    o.status                = "reserved";
    o.productionQty         = 0;
    o.totalProductionMinutes= 0.0;
    o.queuedAt              = "";
    o.productionStartedAt   = "";
    o.estimatedCompletionAt = "";
    o.createdAt             = "2026-05-08T10:00:00";

    Order result = orderFromJson(orderToJson(o));
    EXPECT_EQ(result.orderId, o.orderId);
    EXPECT_EQ(result.sampleId, o.sampleId);
    EXPECT_EQ(result.customerName, o.customerName);
    EXPECT_EQ(result.orderQty, o.orderQty);
    EXPECT_EQ(result.status, o.status);
    EXPECT_DOUBLE_EQ(result.totalProductionMinutes, o.totalProductionMinutes);
}

TEST_F(OrderSerializationTest, EmptyTimestampsRoundTrip) {
    Order o;
    o.orderId = "ORD-20260508-0002"; o.sampleId = "S0001";
    o.customerName = "테스트"; o.orderQty = 5; o.status = "reserved";
    o.productionQty = 0; o.totalProductionMinutes = 0.0;
    o.queuedAt = ""; o.productionStartedAt = ""; o.estimatedCompletionAt = "";
    o.createdAt = "";

    Order result = orderFromJson(orderToJson(o));
    EXPECT_EQ(result.queuedAt, "");
    EXPECT_EQ(result.productionStartedAt, "");
    EXPECT_EQ(result.estimatedCompletionAt, "");
}

// ============================================================
// OrderRepository CRUD 테스트
// ============================================================
class OrderRepositoryTest : public ::testing::Test {
protected:
    std::string filePath    = "test_orders_temp.json";
    std::string seqFilePath = "test_order_seq_temp.json";
    TempFile    tempFile{ filePath };
    TempFile    tempSeq{ seqFilePath };

    std::unique_ptr<JsonRepository<Order>> repo() {
        return makeOrderRepository(filePath, seqFilePath);
    }

    Order makeOrder(const std::string& sampleId = "S0001") {
        Order o;
        o.sampleId = sampleId; o.customerName = "테스트고객";
        o.orderQty = 10; o.status = "reserved";
        o.productionQty = 0; o.totalProductionMinutes = 0.0;
        o.queuedAt = ""; o.productionStartedAt = "";
        o.estimatedCompletionAt = ""; o.createdAt = "2026-05-08T10:00:00";
        return o;
    }
};

TEST_F(OrderRepositoryTest, AddAssignsOrderId) {
    auto r = repo();
    Order o = makeOrder();
    r->add(o);

    EXPECT_THAT(o.orderId, ::testing::StartsWith("ORD-"));
}

TEST_F(OrderRepositoryTest, ConsecutiveAddsIncrementSequence) {
    auto r = repo();
    Order o1 = makeOrder(); r->add(o1);
    Order o2 = makeOrder(); r->add(o2);

    // orderId는 ORD-YYYYMMDD-NNNN 형식, 두 번째 시퀀스가 첫 번째보다 크다
    std::string seq1 = o1.orderId.substr(o1.orderId.rfind('-') + 1);
    std::string seq2 = o2.orderId.substr(o2.orderId.rfind('-') + 1);
    EXPECT_GT(std::stoi(seq2), std::stoi(seq1));
}

TEST_F(OrderRepositoryTest, UpdateStatusPersists) {
    auto r = repo();
    Order o = makeOrder(); r->add(o);

    o.status = "confirmed";
    r->update(o);

    auto found = r->getById(o.orderId);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->status, "confirmed");
}
