#pragma once
#include "JsonRepository.hpp"
#include "../Model/Sample.hpp"
#include <memory>

inline std::unique_ptr<JsonRepository<Sample>> makeSampleRepository(const std::string& filePath) {
    return std::make_unique<JsonRepository<Sample>>(
        filePath,
        [](Sample&) {},                          // setId: no-op (sampleId는 사용자 입력)
        [](const Sample& s) { return s.sampleId; },
        [](const Sample& s) { return sampleToJson(s); },
        [](const json::Value& v) { return sampleFromJson(v); }
    );
}
