#pragma once
#include <string>
#include <JsonLib/json.hpp>

struct Sample {
    std::string sampleId;
    std::string name;
    double      avgProductionTime = 0.0; // 개당 생산시간 (min/ea, 0.1~2.0)
    double      yield             = 0.0; // 수율 0.60~1.00
    int         stockQty          = 0;   // 현재 재고 (0 이상)
};

inline json::Value sampleToJson(const Sample& s) {
    auto obj = json::makeObject();
    obj["sampleId"]           = s.sampleId;
    obj["name"]               = s.name;
    obj["avgProductionTime"]  = s.avgProductionTime;
    obj["yield"]              = s.yield;
    obj["stockQty"]           = s.stockQty;
    return obj;
}

inline Sample sampleFromJson(const json::Value& v) {
    Sample s;
    s.sampleId          = v.at("sampleId").asString();
    s.name              = v.at("name").asString();
    s.avgProductionTime = v.at("avgProductionTime").asDouble();
    s.yield             = v.at("yield").asDouble();
    s.stockQty          = v.contains("stockQty") ? v.at("stockQty").asInt() : 0;
    return s;
}
