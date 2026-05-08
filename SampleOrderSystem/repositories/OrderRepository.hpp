#pragma once
#include "JsonRepository.hpp"
#include "../Model/Order.hpp"
#include "../Utils/TimeUtils.hpp"
#include <JsonLib/json.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace {

inline int readAndIncrementSeq(const std::string& seqFilePath) {
    namespace fs = std::filesystem;
    int seq = 0;
    if (fs::exists(seqFilePath)) {
        auto v = json::Value::parseFile(seqFilePath);
        seq = v.at("seq").asInt();
    }
    seq += 1;
    auto obj = json::makeObject();
    obj["seq"] = seq;
    obj.save(seqFilePath, 0);
    return seq;
}

inline std::string formatSeq(int n) {
    std::string s = std::to_string(n);
    while (s.size() < 4) s = "0" + s;
    return s;
}

} // anonymous namespace

inline std::unique_ptr<JsonRepository<Order>> makeOrderRepository(
    const std::string& filePath,
    const std::string& seqFilePath)
{
    return std::make_unique<JsonRepository<Order>>(
        filePath,
        [seqFilePath](Order& o) {
            int seq = readAndIncrementSeq(seqFilePath);
            o.orderId = "ORD-" + TimeUtils::nowDateYYYYMMDD() + "-" + formatSeq(seq);
        },
        [](const Order& o) { return o.orderId; },
        [](const Order& o) { return orderToJson(o); },
        [](const json::Value& v) { return orderFromJson(v); }
    );
}
