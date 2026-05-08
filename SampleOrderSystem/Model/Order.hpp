#pragma once
#include <string>
#include <JsonLib/json.hpp>

struct Order {
    std::string orderId;               // ORD-YYYYMMDD-NNNN, 자동 부여
    std::string sampleId;
    std::string customerName;
    int         orderQty;
    std::string status;                // reserved/confirmed/producing/released/rejected
    int         productionQty;         // 실생산 수량
    double      totalProductionMinutes;// 총 생산 시간 (분)
    std::string queuedAt;             // ISO 8601, 없으면 ""
    std::string productionStartedAt;  // ISO 8601, 없으면 ""
    std::string estimatedCompletionAt;// ISO 8601, 없으면 ""
    std::string createdAt;            // ISO 8601
};

inline json::Value orderToJson(const Order& o) {
    auto obj = json::makeObject();
    obj["orderId"]                = o.orderId;
    obj["sampleId"]               = o.sampleId;
    obj["customerName"]           = o.customerName;
    obj["orderQty"]               = o.orderQty;
    obj["status"]                 = o.status;
    obj["productionQty"]          = o.productionQty;
    obj["totalProductionMinutes"] = o.totalProductionMinutes;
    obj["queuedAt"]               = o.queuedAt;
    obj["productionStartedAt"]    = o.productionStartedAt;
    obj["estimatedCompletionAt"]  = o.estimatedCompletionAt;
    obj["createdAt"]              = o.createdAt;
    return obj;
}

inline Order orderFromJson(const json::Value& v) {
    Order o;
    o.orderId               = v.at("orderId").asString();
    o.sampleId              = v.at("sampleId").asString();
    o.customerName          = v.at("customerName").asString();
    o.orderQty              = v.at("orderQty").asInt();
    o.status                = v.at("status").asString();
    o.productionQty         = v.contains("productionQty")          ? v.at("productionQty").asInt()          : 0;
    o.totalProductionMinutes= v.contains("totalProductionMinutes") ? v.at("totalProductionMinutes").asDouble(): 0.0;
    o.queuedAt              = v.contains("queuedAt")               ? v.at("queuedAt").asString()             : "";
    o.productionStartedAt   = v.contains("productionStartedAt")    ? v.at("productionStartedAt").asString()  : "";
    o.estimatedCompletionAt = v.contains("estimatedCompletionAt")  ? v.at("estimatedCompletionAt").asString(): "";
    o.createdAt             = v.contains("createdAt")              ? v.at("createdAt").asString()            : "";
    return o;
}
