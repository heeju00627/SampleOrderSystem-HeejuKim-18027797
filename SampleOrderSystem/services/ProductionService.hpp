#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <stdexcept>

#include "../clock/IClock.h"
#include "../Model/Order.hpp"
#include "../Model/Sample.hpp"
#include "../repositories/IRepository.hpp"
#include "../Utils/TimeUtils.hpp"

class ProductionService {
public:
    ProductionService(std::shared_ptr<IClock>               clock,
                      std::shared_ptr<IRepository<Order>>   orderRepo,
                      std::shared_ptr<IRepository<Sample>>  sampleRepo)
        : clock_(std::move(clock))
        , orderRepo_(std::move(orderRepo))
        , sampleRepo_(std::move(sampleRepo)) {}

    // 프로그램 시작 시 및 각 메뉴 진입 시 호출.
    // producing 주문 중 완료된 것을 confirmed로 전이하고, 대기 중인 다음 주문을 시작한다.
    void applyLazyUpdates() {
        bool anyCompleted = true;
        while (anyCompleted) {
            anyCompleted = false;
            auto all = orderRepo_->getAll();

            // 1. 실제 생산 중(productionStartedAt 설정)인 주문 조회
            Order* active = nullptr;
            for (auto& o : all) {
                if (o.status == "producing" && !o.productionStartedAt.empty()) {
                    active = &o;
                    break;
                }
            }

            // 2. 완료 여부 판정
            if (active && completionAt(*active) <= clock_->now()) {
                active->status = "confirmed";
                orderRepo_->update(*active);
                anyCompleted = true;

                // 3~4. 슬롯이 비었으므로 대기 주문 시작
                startNextInQueue(all, completionAt(*active));
            } else if (!active) {
                // 생산 중인 주문이 없을 때도 대기 중인 주문이 있으면 시작
                startNextInQueue(all, clock_->now());
            }
        }
    }

    // 현재 생산 중인 주문 (productionStartedAt 설정, producing 상태)
    std::optional<Order> getActiveOrder() const {
        for (const auto& o : orderRepo_->getAll())
            if (o.status == "producing" && !o.productionStartedAt.empty())
                return o;
        return std::nullopt;
    }

    // 대기 중인 주문 목록 (productionStartedAt 비어 있음, queuedAt 오름차순)
    std::vector<Order> getQueue() const {
        std::vector<Order> queue;
        for (const auto& o : orderRepo_->getAll())
            if (o.status == "producing" && o.productionStartedAt.empty())
                queue.push_back(o);
        std::sort(queue.begin(), queue.end(),
            [](const Order& a, const Order& b) {
                return a.queuedAt < b.queuedAt;
            });
        return queue;
    }

    // 잔여 생산 시간 (분). 음수이면 0 반환.
    double getRemainingMinutes(const std::string& orderId) const {
        auto opt = orderRepo_->getById(orderId);
        if (!opt || opt->estimatedCompletionAt.empty()) return 0.0;

        auto endTp = parseIso8601(opt->estimatedCompletionAt);
        auto nowTp = clock_->now();
        if (endTp <= nowTp) return 0.0;

        auto diff = std::chrono::duration_cast<
            std::chrono::duration<double, std::ratio<60>>>(endTp - nowTp);
        return diff.count();
    }

private:
    std::shared_ptr<IClock>              clock_;
    std::shared_ptr<IRepository<Order>>  orderRepo_;
    std::shared_ptr<IRepository<Sample>> sampleRepo_;

    // ISO 8601 문자열 → time_point
    static std::chrono::system_clock::time_point parseIso8601(const std::string& s) {
        std::tm tm{};
        int y, mo, d, h, mi, sec;
        if (std::sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &sec) == 6) {
            tm.tm_year = y - 1900; tm.tm_mon = mo - 1; tm.tm_mday = d;
            tm.tm_hour = h; tm.tm_min = mi; tm.tm_sec = sec;
            tm.tm_isdst = -1;
#ifdef _WIN32
            return std::chrono::system_clock::from_time_t(_mkgmtime(&tm));
#else
            return std::chrono::system_clock::from_time_t(timegm(&tm));
#endif
        }
        return std::chrono::system_clock::time_point{};
    }

    std::chrono::system_clock::time_point completionAt(const Order& o) const {
        return parseIso8601(o.estimatedCompletionAt);
    }

    // 대기 목록에서 다음 주문을 꺼내 생산 시작 처리
    void startNextInQueue(std::vector<Order>& all,
                          std::chrono::system_clock::time_point prevCompletedAt) {
        // 대기 중인 주문을 queuedAt 순으로 정렬
        std::vector<Order*> waiting;
        for (auto& o : all)
            if (o.status == "producing" && o.productionStartedAt.empty())
                waiting.push_back(&o);
        if (waiting.empty()) return;

        std::sort(waiting.begin(), waiting.end(),
            [](const Order* a, const Order* b) { return a->queuedAt < b->queuedAt; });

        Order* next = waiting.front();
        // 시각 역행 방지: now()와 직전 완료 시각 중 늦은 것을 시작 시각으로
        auto startTp = std::max(clock_->now(), prevCompletedAt);
        auto endTp   = startTp + std::chrono::duration_cast<
            std::chrono::system_clock::duration>(
                std::chrono::duration<double, std::ratio<60>>(
                    next->totalProductionMinutes));

        next->productionStartedAt   = TimeUtils::toIso8601(startTp);
        next->estimatedCompletionAt = TimeUtils::toIso8601(endTp);
        orderRepo_->update(*next);
    }
};
