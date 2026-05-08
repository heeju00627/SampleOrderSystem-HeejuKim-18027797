#pragma once
#include "IClock.h"

class MockClock : public IClock {
public:
    explicit MockClock(std::chrono::system_clock::time_point start
                       = std::chrono::system_clock::now())
        : current_(start) {}

    std::chrono::system_clock::time_point now() const override {
        return current_;
    }

    void setNow(std::chrono::system_clock::time_point tp) {
        current_ = tp;
    }

    void advance(double minutes) {
        current_ += std::chrono::duration_cast<std::chrono::system_clock::duration>(
            std::chrono::duration<double, std::ratio<60>>(minutes));
    }

private:
    std::chrono::system_clock::time_point current_;
};
