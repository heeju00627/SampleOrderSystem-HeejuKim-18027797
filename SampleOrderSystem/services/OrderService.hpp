#pragma once
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

#include "../Model/Order.hpp"
#include "../Model/Sample.hpp"
#include "../repositories/IRepository.hpp"
#include "../services/ProductionCalculator.hpp"
#include "../Utils/TimeUtils.hpp"

class OrderService {
public:
    OrderService(std::shared_ptr<IRepository<Sample>> sampleRepo,
                 std::shared_ptr<IRepository<Order>>  orderRepo)
        : sampleRepo_(std::move(sampleRepo))
        , orderRepo_(std::move(orderRepo)) {}

    Order placeOrder(const std::string& sampleId,
                     const std::string& customerName,
                     int orderQty) {
        if (!sampleRepo_->getById(sampleId).has_value())
            throw std::invalid_argument("존재하지 않는 시료: " + sampleId);

        Order o;
        o.sampleId      = sampleId;
        o.customerName  = customerName;
        o.orderQty      = orderQty;
        o.status        = "reserved";
        o.productionQty = 0;
        o.totalProductionMinutes = 0.0;
        o.createdAt     = TimeUtils::nowIso8601();
        orderRepo_->add(o);
        return o;
    }

    void approve(const std::string& orderId) {
        Order o = getOrderOrThrow(orderId);
        if (o.status != "reserved")
            throw std::invalid_argument("reserved 상태 주문만 승인 가능합니다. 현재: " + o.status);

        Sample s = getSampleOrThrow(o.sampleId);

        if (s.stockQty >= o.orderQty) {
            // 재고 충분 → confirmed
            s.stockQty -= o.orderQty;
            o.status = "confirmed";
        } else {
            // 재고 부족 → producing
            int qty = ProductionCalculator::calcProductionQty(o.orderQty, s.stockQty, s.yield);
            double totalMin = ProductionCalculator::calcTotalMinutes(qty, s.avgProductionTime);
            o.productionQty          = qty;
            o.totalProductionMinutes = totalMin;
            o.queuedAt               = TimeUtils::nowIso8601();
            o.status                 = "producing";
            s.stockQty               = 0; // 가용 재고 전량 선점
            // productionStartedAt, estimatedCompletionAt은 Phase 4 ProductionService에서 설정
        }

        sampleRepo_->update(s);
        orderRepo_->update(o);
    }

    void reject(const std::string& orderId) {
        Order o = getOrderOrThrow(orderId);
        if (o.status != "reserved")
            throw std::invalid_argument("reserved 상태 주문만 거절 가능합니다. 현재: " + o.status);
        o.status = "rejected";
        orderRepo_->update(o);
    }

    void release(const std::string& orderId) {
        Order o = getOrderOrThrow(orderId);
        if (o.status != "confirmed")
            throw std::invalid_argument("confirmed 상태 주문만 출고 처리 가능합니다. 현재: " + o.status);
        o.status = "released";
        orderRepo_->update(o);
    }

    std::vector<Order> findByStatus(const std::string& status) const {
        auto all = orderRepo_->getAll();
        std::vector<Order> result;
        for (const auto& o : all)
            if (o.status == status) result.push_back(o);
        return result;
    }

    std::optional<Order> findById(const std::string& orderId) const {
        return orderRepo_->getById(orderId);
    }

private:
    std::shared_ptr<IRepository<Sample>> sampleRepo_;
    std::shared_ptr<IRepository<Order>>  orderRepo_;

    Order getOrderOrThrow(const std::string& orderId) const {
        auto opt = orderRepo_->getById(orderId);
        if (!opt) throw std::invalid_argument("주문을 찾을 수 없음: " + orderId);
        return *opt;
    }

    Sample getSampleOrThrow(const std::string& sampleId) const {
        auto opt = sampleRepo_->getById(sampleId);
        if (!opt) throw std::invalid_argument("시료를 찾을 수 없음: " + sampleId);
        return *opt;
    }
};
