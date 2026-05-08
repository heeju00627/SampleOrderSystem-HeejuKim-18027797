#pragma once
#include "IController.h"
#include "UIHelpers.hpp"
#include "../View/ConsoleView.h"
#include "../services/SampleService.hpp"
#include "../services/OrderService.hpp"
#include "../services/ProductionService.hpp"
#include "../View/ConsoleUI.h"
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

// ── 재고 상태 → 색상 (ConsoleUI 의존, 테스트 제외) ───────────
inline ConsoleUI::Color stockStatusColor(StockStatus::Value v) {
    switch (v) {
        case StockStatus::Depleted: return ConsoleUI::Color::Red;
        case StockStatus::Short:    return ConsoleUI::Color::Yellow;
        default:                    return ConsoleUI::Color::Green;
    }
}

// ── 주문 상태 → 색상 ─────────────────────────────────────────
inline ConsoleUI::Color statusColor(const std::string& status) {
    if (status == "confirmed") return ConsoleUI::Color::Green;
    if (status == "producing") return ConsoleUI::Color::Cyan;
    if (status == "released")  return ConsoleUI::Color::Gray;
    if (status == "rejected")  return ConsoleUI::Color::Red;
    return ConsoleUI::Color::White;
}

// ─────────────────────────────────────────────────────────────
class OrderController : public Controller::IController {
public:
    OrderController(std::shared_ptr<SampleService>     sampleSvc,
                    std::shared_ptr<OrderService>      orderSvc,
                    std::shared_ptr<ProductionService> prodSvc)
        : sampleSvc_(std::move(sampleSvc))
        , orderSvc_(std::move(orderSvc))
        , prodSvc_(std::move(prodSvc)) {}

    void run() override {
        running_ = true;
        while (running_) {
            ConsoleUI::printHeader("S-Semi 생산주문관리 시스템");
            ConsoleUI::printMenuItem(1, "시료 관리");
            ConsoleUI::printMenuItem(2, "시료 주문");
            ConsoleUI::printMenuItem(3, "주문 승인/거절");
            ConsoleUI::printMenuItem(4, "모니터링");
            ConsoleUI::printMenuItem(5, "생산 라인 조회");
            ConsoleUI::printMenuItem(6, "출고 처리");
            ConsoleUI::printMenuItem(0, "종료", true);
            std::cout << '\n';

            int choice = ConsoleUI::getIntInput("  선택 > ");
            handleCommand(std::to_string(choice), {});
        }
    }

    bool handleCommand(const std::string& command,
                       const std::vector<std::string>&) override {
        int n = 0;
        try { n = std::stoi(command); } catch (...) { return true; }

        prodSvc_->applyLazyUpdates(); // 모든 메뉴 진입 시 lazy 갱신

        switch (n) {
            case 1: handleSampleMenu();    break;
            case 2: handlePlaceOrder();    break;
            case 3: handleApproveReject(); break;
            case 4: handleMonitor();       break;
            case 5: handleProductionLine();break;
            case 6: handleRelease();       break;
            case 0: running_ = false;      break;
            default: ConsoleUI::printError("잘못된 메뉴 번호입니다."); break;
        }
        return running_;
    }

private:
    std::shared_ptr<SampleService>     sampleSvc_;
    std::shared_ptr<OrderService>      orderSvc_;
    std::shared_ptr<ProductionService> prodSvc_;
    bool running_ = false;

    // ── 1. 시료 관리 ────────────────────────────────────────
    void handleSampleMenu() {
        while (true) {
            ConsoleUI::printSubHeader("시료 관리");
            ConsoleUI::printMenuItem(1, "시료 목록 조회");
            ConsoleUI::printMenuItem(2, "시료 검색");
            ConsoleUI::printMenuItem(3, "시료 등록");
            ConsoleUI::printMenuItem(0, "돌아가기", true);
            std::cout << '\n';

            int choice = ConsoleUI::getIntInput("  선택 > ");
            if (choice == 0) break;
            prodSvc_->applyLazyUpdates();

            if (choice == 1) showSampleList();
            else if (choice == 2) searchSample();
            else if (choice == 3) registerSample();
            ConsoleUI::pressEnterToContinue();
        }
    }

    void showSampleList() {
        ConsoleUI::printSubHeader("시료 목록");
        auto samples = sampleSvc_->findAll();
        std::vector<std::string> headers = { "시료ID", "이름", "min/ea", "수율", "재고", "상태" };
        std::vector<std::vector<std::string>> rows;
        for (const auto& s : samples) {
            int demand = calcReservedDemand(s.sampleId);
            auto sv = StockStatus::evaluate(s.stockQty, demand);
            rows.push_back({
                s.sampleId, s.name,
                formatDouble(s.avgProductionTime),
                formatDouble(s.yield),
                std::to_string(s.stockQty),
                StockStatus::toString(sv)
            });
        }
        //           시료ID  이름   min/ea  수율  재고  상태
        ConsoleUI::printTable(headers, rows, { 8, 20, 8, 6, 6, 6 });
    }

    void searchSample() {
        std::string kw = ConsoleUI::getStringInput("  키워드 > ");
        auto samples = sampleSvc_->search(kw);
        ConsoleUI::printSubHeader("검색 결과: " + kw);
        std::vector<std::string> headers = { "시료ID", "이름", "min/ea", "수율", "재고" };
        std::vector<std::vector<std::string>> rows;
        for (const auto& s : samples)
            rows.push_back({ s.sampleId, s.name,
                formatDouble(s.avgProductionTime), formatDouble(s.yield),
                std::to_string(s.stockQty) });
        ConsoleUI::printTable(headers, rows, { 8, 20, 8, 6, 6 });
    }

    void registerSample() {
        ConsoleUI::printSubHeader("시료 등록");
        Sample s;
        s.sampleId          = ConsoleUI::getStringInput("  시료ID      > ");
        s.name              = ConsoleUI::getStringInput("  이름        > ");
        s.avgProductionTime = getDoubleInput(          "  평균생산시간(min/ea) > ");
        s.yield             = getDoubleInput(          "  수율(0.0~1.0) > ");
        s.stockQty          = 0;
        try {
            sampleSvc_->create(s);
            ConsoleUI::printSuccess("시료 등록 완료: " + s.sampleId);
        } catch (const std::exception& e) {
            ConsoleUI::printError(e.what());
        }
    }

    // ── 2. 시료 주문 ────────────────────────────────────────
    void handlePlaceOrder() {
        ConsoleUI::printSubHeader("시료 주문");
        showSampleList();
        std::cout << '\n';
        std::string sampleId   = ConsoleUI::getStringInput("  시료ID  > ");
        std::string customerName = ConsoleUI::getStringInput("  고객명  > ");
        int qty = ConsoleUI::getIntInput("  주문수량 > ");
        try {
            auto order = orderSvc_->placeOrder(sampleId, customerName, qty);
            ConsoleUI::printSuccess("주문 접수 완료: " + order.orderId + " (reserved)");
        } catch (const std::exception& e) {
            ConsoleUI::printError(e.what());
        }
        ConsoleUI::pressEnterToContinue();
    }

    // ── 3. 주문 승인/거절 ────────────────────────────────────
    void handleApproveReject() {
        ConsoleUI::printSubHeader("주문 승인/거절");
        auto reserved = orderSvc_->findByStatus("reserved");
        if (reserved.empty()) { ConsoleUI::printEmpty(); ConsoleUI::pressEnterToContinue(); return; }

        std::vector<std::string> headers = { "주문ID", "시료ID", "고객명", "수량" };
        std::vector<std::vector<std::string>> rows;
        for (const auto& o : reserved)
            rows.push_back({ o.orderId, o.sampleId, o.customerName, std::to_string(o.orderQty) });
        ConsoleUI::printTable(headers, rows, 20);
        std::cout << '\n';

        std::string orderId = ConsoleUI::getStringInput("  주문ID > ");
        if (orderId.empty()) { ConsoleUI::pressEnterToContinue(); return; }

        bool approve = ConsoleUI::getYNInput("  [Y] 승인 / [N] 거절");
        if (approve) {
            try {
                orderSvc_->approve(orderId);
                auto opt = orderSvc_->findById(orderId);
                if (opt && opt->status == "confirmed")
                    ConsoleUI::printSuccess("승인 완료 — 출고 대기 (confirmed)");
                else
                    ConsoleUI::printSuccess("승인 완료 — 생산 등록 (producing)");
            } catch (const std::exception& e) {
                ConsoleUI::printError(e.what());
            }
        } else {
            if (ConsoleUI::getYNInput("  정말 거절하시겠습니까?")) {
                try {
                    orderSvc_->reject(orderId);
                    ConsoleUI::printSuccess("거절 완료 (rejected)");
                } catch (const std::exception& e) {
                    ConsoleUI::printError(e.what());
                }
            } else {
                ConsoleUI::printInfo("취소됐습니다.");
            }
        }
        ConsoleUI::pressEnterToContinue();
    }

    // ── 4. 모니터링 ─────────────────────────────────────────
    void handleMonitor() {
        ConsoleUI::printSubHeader("모니터링");
        ConsoleUI::printMenuItem(1, "주문 상태별 목록");
        ConsoleUI::printMenuItem(2, "시료별 재고 현황");
        ConsoleUI::printMenuItem(0, "돌아가기", true);
        std::cout << '\n';

        int choice = ConsoleUI::getIntInput("  선택 > ");
        if (choice == 1) showOrdersByStatus();
        else if (choice == 2) showInventory();
        ConsoleUI::pressEnterToContinue();
    }

    void showOrdersByStatus() {
        const std::vector<std::string> statuses = { "reserved", "confirmed", "producing", "released" };
        const std::vector<std::string> headers = { "주문ID", "시료ID", "고객명", "수량", "상태" };
        for (const auto& st : statuses) {
            auto orders = orderSvc_->findByStatus(st);
            ConsoleUI::printSubHeader(st + " (" + std::to_string(orders.size()) + "건)");
            std::vector<std::vector<std::string>> rows;
            for (const auto& o : orders)
                rows.push_back({ o.orderId, o.sampleId, o.customerName,
                                 std::to_string(o.orderQty), o.status });
            ConsoleUI::printTable(headers, rows, 20);
        }
    }

    void showInventory() {
        ConsoleUI::printSubHeader("시료별 재고 현황");
        ConsoleUI::printInfo("※ 수요: 승인 대기 중(reserved) 주문 합계  |  재고는 승인 시 자동 차감됨");
        auto samples = sampleSvc_->findAll();
        std::vector<std::string> headers = { "시료ID", "이름", "재고", "예약수요", "잔여율", "상태" };
        std::vector<std::vector<std::string>> rows;
        for (const auto& s : samples) {
            int demand = calcReservedDemand(s.sampleId);
            auto sv = StockStatus::evaluate(s.stockQty, demand);
            std::string ratio;
            if (s.stockQty == 0)
                ratio = "0%";
            else if (demand == 0)
                ratio = "-";
            else
                ratio = std::to_string(s.stockQty * 100 / demand) + "%";
            rows.push_back({ s.sampleId, s.name,
                             std::to_string(s.stockQty),
                             demand > 0 ? std::to_string(demand) : "-",
                             ratio,
                             StockStatus::toString(sv) });
        }
        //           시료ID  이름   재고  예약수요  잔여율  상태
        ConsoleUI::printTable(headers, rows, { 8, 20, 6, 8, 8, 6 });
    }

    // ── 5. 생산 라인 조회 ────────────────────────────────────
    void handleProductionLine() {
        ConsoleUI::printSubHeader("생산 중인 주문");
        auto active = prodSvc_->getActiveOrder();
        if (!active) {
            ConsoleUI::printEmpty();
        } else {
            double rem = prodSvc_->getRemainingMinutes(active->orderId);
            std::string remStr = formatRemainingTime(rem);
            std::vector<std::string> hdr = { "주문ID", "시료", "고객", "수량", "시작시각", "예상완료", "잔여" };
            ConsoleUI::printTable(hdr, {{ active->orderId, active->sampleId,
                active->customerName, std::to_string(active->orderQty),
                active->productionStartedAt, active->estimatedCompletionAt, remStr }}, 20);
        }

        ConsoleUI::printSubHeader("생산 대기 중 (FIFO)");
        auto queue = prodSvc_->getQueue();
        if (queue.empty()) {
            ConsoleUI::printEmpty();
        } else {
            std::vector<std::string> hdr2 = { "순번", "주문ID", "시료", "고객", "수량", "진입시각" };
            std::vector<std::vector<std::string>> rows;
            int i = 1;
            for (const auto& o : queue)
                rows.push_back({ std::to_string(i++), o.orderId, o.sampleId,
                                 o.customerName, std::to_string(o.orderQty), o.queuedAt });
            ConsoleUI::printTable(hdr2, rows, 20);
        }
        ConsoleUI::pressEnterToContinue();
    }

    // ── 6. 출고 처리 ────────────────────────────────────────
    void handleRelease() {
        ConsoleUI::printSubHeader("출고 처리");
        auto confirmed = orderSvc_->findByStatus("confirmed");
        if (confirmed.empty()) { ConsoleUI::printEmpty(); ConsoleUI::pressEnterToContinue(); return; }

        std::vector<std::string> headers = { "주문ID", "시료ID", "고객명", "수량" };
        std::vector<std::vector<std::string>> rows;
        for (const auto& o : confirmed)
            rows.push_back({ o.orderId, o.sampleId, o.customerName, std::to_string(o.orderQty) });
        ConsoleUI::printTable(headers, rows, 20);
        std::cout << '\n';

        std::string orderId = ConsoleUI::getStringInput("  주문ID > ");
        if (orderId.empty()) { ConsoleUI::pressEnterToContinue(); return; }

        if (ConsoleUI::getYNInput("  출고 처리하시겠습니까?")) {
            try {
                orderSvc_->release(orderId);
                ConsoleUI::printSuccess("출고 완료: " + orderId + " (released)");
            } catch (const std::exception& e) {
                ConsoleUI::printError(e.what());
            }
        } else {
            ConsoleUI::printInfo("취소됐습니다.");
        }
        ConsoleUI::pressEnterToContinue();
    }

    // ── 헬퍼 ────────────────────────────────────────────────
    // confirmed/producing 주문은 approve() 시 stockQty에서 이미 차감됨.
    // 미확정(reserved) 주문만 "미래 수요"로 집계한다.
    int calcReservedDemand(const std::string& sampleId) const {
        int total = 0;
        for (const auto& o : orderSvc_->findByStatus("reserved"))
            if (o.sampleId == sampleId) total += o.orderQty;
        return total;
    }

    static std::string formatDouble(double v) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << v;
        return ss.str();
    }

    static double getDoubleInput(const std::string& prompt) {
        while (true) {
            std::string s = ConsoleUI::getStringInput(prompt);
            try { return std::stod(s); }
            catch (...) { ConsoleUI::printError("올바른 숫자를 입력해주세요."); }
        }
    }
};
