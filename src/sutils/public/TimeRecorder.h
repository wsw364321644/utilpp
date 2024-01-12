#pragma once
#include <stdint.h>
#include <chrono>
class FTimeRecorder {
public:
    FTimeRecorder();
    void Tick();
    float GetDeltaSec() const;

private:
    std::chrono::steady_clock::time_point LastTickTime;
    std::chrono::steady_clock::time_point CurTickTime;
};