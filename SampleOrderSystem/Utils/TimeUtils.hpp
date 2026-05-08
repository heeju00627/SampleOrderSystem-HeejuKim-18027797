#pragma once
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace TimeUtils {

inline std::string toIso8601(const std::chrono::system_clock::time_point& tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm_buf);
    return buf;
}

inline std::string nowIso8601() {
    return toIso8601(std::chrono::system_clock::now());
}

inline std::string nowDateYYYYMMDD() {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y%m%d", &tm_buf);
    return buf;
}

} // namespace TimeUtils
