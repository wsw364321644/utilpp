#pragma once
#include <stdint.h>
#include <chrono>
#include <std_ext.h>
class FTimeRecorder {
public:
    FTimeRecorder();
    void Tick();
    float GetDeltaSec() const;
    template<class T ,std::enable_if_t<is_specialization_v<T,std::chrono::duration>,int> = 0>
    [[nodiscard]] constexpr T GetDelta()const {
        return std::chrono::duration_cast<T>(CurTickTime - LastTickTime);
    }
    template<class T, std::enable_if_t<is_specialization_v<T, std::chrono::duration>, int> = 0>
    [[nodiscard]] T GetDeltaToNow()const {
        return std::chrono::duration_cast<T>(std::chrono::steady_clock::now() - CurTickTime);
    }
private:
    std::chrono::steady_clock::time_point LastTickTime;
    std::chrono::steady_clock::time_point CurTickTime;
};