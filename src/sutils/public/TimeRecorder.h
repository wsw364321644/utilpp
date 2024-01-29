#pragma once
#include <stdint.h>
#include <chrono>
#include <std_ext.h>
class FTimeRecorder {
public:
    FTimeRecorder();
    void Tick();
    void RecordBegin();
    void RecordEnd();
    float GetDeltaSec() const;
    template<class T ,std::enable_if_t<is_specialization_v<T,std::chrono::duration>,int> = 0>
    [[nodiscard]] constexpr T GetDelta()const {
        return std::chrono::duration_cast<T>(CurTickTime - LastTickTime);
    }
private:
    std::chrono::steady_clock::time_point LastTickTime;
    std::chrono::steady_clock::time_point CurTickTime;
};