#pragma once
#include <cmath>

class ProductionCalculator {
public:
    // 실생산 수량: ceil(부족분 / (yield * 0.9))
    static int calcProductionQty(int orderQty, int stockQty, double yield) {
        int shortage = orderQty - stockQty;
        if (shortage <= 0) return 0;
        return static_cast<int>(std::ceil(shortage / (yield * 0.9)));
    }

    // 총 생산 시간 (분): productionQty * avgProductionTime
    static double calcTotalMinutes(int productionQty, double avgProductionTime) {
        return static_cast<double>(productionQty) * avgProductionTime;
    }
};
