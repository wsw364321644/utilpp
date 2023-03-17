/**
 *  rate_calc.h
 */
#pragma once

#include <stdint.h>
#include <array>

const uint32_t RateSlotSize = 25;

struct SRateSlot
{
    uint32_t uDownloadSize;
    int32_t iMilliSec;
};

class CRateCalc
{
public:
    CRateCalc(){};
    ~CRateCalc(){};

    void Init()
    {
        m_uLastDownloadSize = 0;
        m_uSlotPos = 0;
        m_uDownRate = 0;

        for (uint32_t i = 0; i < RateSlotSize; ++i)
        {
            m_RateSlots[i].uDownloadSize = 0;
            m_RateSlots[i].iMilliSec = 0;
        }
    }

    void SnapShots(uint64_t currentDownloadSize, int32_t iMillSec)
    {
        m_RateSlots[m_uSlotPos].uDownloadSize = static_cast<uint32_t>((currentDownloadSize > m_uLastDownloadSize) ? (currentDownloadSize - m_uLastDownloadSize) : 0);
        m_RateSlots[m_uSlotPos].iMilliSec = iMillSec;

        m_uSlotPos++;
        m_uSlotPos %= RateSlotSize;
        m_uLastDownloadSize = currentDownloadSize;

        uint64_t uTotalDownSize = 0;
        uint64_t uTotalTime = 0;

        for (uint8_t i = 0; i < RateSlotSize; ++i)
        {
            uTotalDownSize += m_RateSlots[i].uDownloadSize;
            uTotalTime += m_RateSlots[i].iMilliSec;
        }
        m_uDownRate = static_cast<int32_t>((uTotalDownSize * 1000) / (uTotalTime + 1));
    }

    uint32_t GetDownRate() const
    {
        return m_uDownRate;
    }

private:
    std::array<SRateSlot, RateSlotSize> m_RateSlots;
    uint8_t m_uSlotPos;
    uint32_t m_uDownRate;
    uint64_t m_uLastDownloadSize;
};
