#pragma once
#include <string>
#include <cmath>

// ── 재고 상태 판단 (순수 함수, OS 의존 없음) ──────────────────
struct StockStatus {
    enum Value { Depleted, Short, Sufficient };

    static Value evaluate(int stockQty, int totalDemand) {
        if (stockQty <= 0)          return Depleted;
        if (stockQty < totalDemand) return Short;
        return Sufficient;
    }

    static std::string toString(Value v) {
        switch (v) {
            case Depleted:  return "고갈";
            case Short:     return "부족";
            default:        return "여유";
        }
    }
};

// ── 잔여 시간 포맷 (순수 함수) ─────────────────────────────────
inline std::string formatRemainingTime(double minutes) {
    int totalMin = static_cast<int>(std::ceil(minutes)); // 분 단위 올림
    if (totalMin <= 0) return "0m";
    int h = totalMin / 60;
    int m = totalMin % 60;
    if (h > 0) return std::to_string(h) + "h " + std::to_string(m) + "m";
    return std::to_string(m) + "m";
}
