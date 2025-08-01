#pragma once
#include <stdint.h>
#include <chrono>
#include <functional>
#include <std_ext.h>
#include "simple_export_ppdefs.h"
class SIMPLE_UTIL_EXPORT FTimeRecorder {
public:
    FTimeRecorder();
    void Tick();
    void Reset();
    float GetDeltaSec() const;
    template<class T ,std::enable_if_t<is_specialization_v<T,std::chrono::duration>,int> = 0>
    [[nodiscard]] constexpr T GetDelta()const {
        return std::chrono::duration_cast<T>(CurTickTime - LastTickTime);
    }
    template<class T, std::enable_if_t<is_specialization_v<T, std::chrono::duration>, int> = 0>
    [[nodiscard]] T GetDeltaToNow()const {
        return std::chrono::duration_cast<T>(std::chrono::steady_clock::now() - CurTickTime);
    }
    template<class T, std::enable_if_t<is_specialization_v<T, std::chrono::duration>, int> = 0>
    [[nodiscard]] T GetTotalTime()const {
        return std::chrono::duration_cast<T>(CurTickTime - StartTime);
    }
private:
    std::chrono::steady_clock::time_point StartTime;
    std::chrono::steady_clock::time_point LastTickTime;
    std::chrono::steady_clock::time_point CurTickTime;
};

class SIMPLE_UTIL_EXPORT FDelayRecorder {
public:
    typedef std::function<void()> FDelayEndDelegate;
    FDelayRecorder();
    void Tick();
    void Tick(float delta);
    template<class T, std::enable_if_t<is_specialization_v<T, std::chrono::duration>, int> = 0>
    void SetDelay(T duration, FDelayEndDelegate Delegate) {
        SetDelay(std::chrono::duration_cast<std::chrono::milliseconds>(duration), std::move(Delegate));
    }
    void SetDelay(std::chrono::milliseconds duration, FDelayEndDelegate delegate);
    std::chrono::milliseconds GetDelay();
    std::chrono::milliseconds GetDelayConfig();
    void Clear();
private:
    FDelayEndDelegate DelayEndDelegate;
    std::chrono::milliseconds Delay{ 0 };
    std::chrono::milliseconds DelayCount{ 0 };
    std::chrono::steady_clock::time_point LastTickTime;
};