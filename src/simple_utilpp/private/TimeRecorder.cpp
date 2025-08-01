#include "TimeRecorder.h"


FTimeRecorder::FTimeRecorder()
{
    CurTickTime = std::chrono::steady_clock::now();
    StartTime = CurTickTime;
}
void FTimeRecorder::Tick()
{
    LastTickTime = CurTickTime;
    CurTickTime = std::chrono::steady_clock::now();
}

void FTimeRecorder::Reset()
{
    StartTime = LastTickTime = CurTickTime = std::chrono::steady_clock::now();
}

float FTimeRecorder::GetDeltaSec() const
{
    return ((float)std::chrono::duration_cast<std::chrono::nanoseconds>(CurTickTime - LastTickTime).count()) / std::chrono::nanoseconds::period::den;
}

FDelayRecorder::FDelayRecorder()
{
}

void FDelayRecorder::Tick()
{
    if (Delay.count() <= DelayCount.count())
        return;
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    auto tickDur = std::chrono::duration_cast<std::chrono::milliseconds>(now - LastTickTime);
    auto fTickDur=tickDur.count() / 1000.0f;
    Tick(fTickDur);
    LastTickTime = now;
}

void FDelayRecorder::Tick(float delta)
{
    if (Delay.count() <= DelayCount.count())
        return;
    DelayCount += std::chrono::milliseconds(int(delta * 1000));
    if (Delay.count() <= DelayCount.count()) {
        if (DelayEndDelegate) {
            DelayEndDelegate();
        }
    }
}

void FDelayRecorder::SetDelay(std::chrono::milliseconds duration, FDelayEndDelegate delegate)
{
    DelayEndDelegate = std::move(delegate);
    Delay = duration;
    LastTickTime= std::chrono::steady_clock::now();
}

std::chrono::milliseconds FDelayRecorder::GetDelay()
{
    return Delay - DelayCount;
}

std::chrono::milliseconds FDelayRecorder::GetDelayConfig()
{
    return Delay;
}

void FDelayRecorder::Clear()
{
    DelayEndDelegate = nullptr;
    Delay = std::chrono::milliseconds(0);
}
