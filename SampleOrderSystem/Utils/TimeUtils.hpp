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
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
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

// ISO 8601 문자열(UTC) → time_point
inline std::chrono::system_clock::time_point parseIso8601(const std::string& s) {
    std::tm tm{};
    int y, mo, d, h, mi, sec;
#ifdef _WIN32
    const int parsed = sscanf_s(s.c_str(), "%d-%d-%dT%d:%d:%d",
                                &y, &mo, &d, &h, &mi, &sec);
#else
    const int parsed = std::sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d",
                                   &y, &mo, &d, &h, &mi, &sec);
#endif
    if (parsed == 6) {
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

} // namespace TimeUtils
