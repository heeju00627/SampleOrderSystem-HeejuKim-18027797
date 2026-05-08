#pragma once
#include "IClock.h"

class SystemClock : public IClock {
public:
    std::chrono::system_clock::time_point now() const override {
        return std::chrono::system_clock::now();
    }
};
