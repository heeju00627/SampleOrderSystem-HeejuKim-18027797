#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../Controller/UIHelpers.hpp"

// ============================================================
// StockStatus 순수 함수 테스트
// ============================================================
class StockStatusTest : public ::testing::Test {};

TEST_F(StockStatusTest, ZeroStockIsDepleted) {
    EXPECT_EQ(StockStatus::evaluate(0, 50), StockStatus::Depleted);
}

TEST_F(StockStatusTest, StockLessThanDemandIsShort) {
    EXPECT_EQ(StockStatus::evaluate(10, 50), StockStatus::Short);
}

TEST_F(StockStatusTest, StockEqualsDemandIsSufficient) {
    EXPECT_EQ(StockStatus::evaluate(50, 50), StockStatus::Sufficient);
}

TEST_F(StockStatusTest, StockExceedsDemandIsSufficient) {
    EXPECT_EQ(StockStatus::evaluate(100, 50), StockStatus::Sufficient);
}

TEST_F(StockStatusTest, ZeroStockZeroDemandIsDepleted) {
    // 재고=0은 고갈이 최우선
    EXPECT_EQ(StockStatus::evaluate(0, 0), StockStatus::Depleted);
}

// ============================================================
// formatRemainingTime 순수 함수 테스트
// ============================================================
class FormatRemainingTimeTest : public ::testing::Test {};

TEST_F(FormatRemainingTimeTest, OverOneHour) {
    EXPECT_EQ(formatRemainingTime(143.0), "2h 23m");
}

TEST_F(FormatRemainingTimeTest, ExactlyOneHour) {
    EXPECT_EQ(formatRemainingTime(60.0), "1h 0m");
}

TEST_F(FormatRemainingTimeTest, UnderOneHour) {
    EXPECT_EQ(formatRemainingTime(45.0), "45m");
}

TEST_F(FormatRemainingTimeTest, Zero) {
    EXPECT_EQ(formatRemainingTime(0.0), "0m");
}

TEST_F(FormatRemainingTimeTest, FractionalCeilingUnderHour) {
    // 30.1분 → ceil → 31m
    EXPECT_EQ(formatRemainingTime(30.1), "31m");
}

TEST_F(FormatRemainingTimeTest, FractionalCeilingOverHour) {
    // 60.5분 → ceil → 61분 → 1h 1m
    EXPECT_EQ(formatRemainingTime(60.5), "1h 1m");
}

TEST_F(FormatRemainingTimeTest, ExactIntegerUnchanged) {
    // 정수 분은 변동 없음
    EXPECT_EQ(formatRemainingTime(45.0), "45m");
}
