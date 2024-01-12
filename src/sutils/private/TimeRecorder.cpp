#include "TimeRecorder.h"


FTimeRecorder::FTimeRecorder()
{
}
void FTimeRecorder::Tick()
{
    LastTickTime = CurTickTime;
    CurTickTime =std::chrono::steady_clock::now();
}
float FTimeRecorder::GetDeltaSec() const
{
    return ((float)std::chrono::duration_cast<std::chrono::nanoseconds>(CurTickTime- LastTickTime).count())/std::chrono::nanoseconds::period::den;
}