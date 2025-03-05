#pragma once
#include <functional>
#include <vector>
#include <set>
#include <numeric>
#include <optional>
#include <handle.h>
#include "Task/TaskManager.h"

typedef struct FinishedSlotData_t {
    uint8_t ID;
    CommonTaskHandle_t Handle;
}FinishedSlotData;
template <typename R>
class FTaskSlotCounter {
public:
    
    FTaskSlotCounter(uint8_t ParallelTaskNum) :Futures(ParallelTaskNum), Handles(ParallelTaskNum){
        for (int i=0; i < ParallelTaskNum; i++) {
            FreeIndex.insert(i);
        }
    }
    bool HasSpace() {
        if (FreeIndex.size() == 0) {
            return false;
        }
        return true;
    }
    bool HasTask() {
        if (UsedIndex.size() > 0) {
            return true;
        }
        return false;
    }
    std::optional<uint8_t>  GetFreeSlot() {
        if (!HasSpace()) {
            return std::nullopt;
        }
        return *FreeIndex.begin();

    }
    bool SetFuture(uint8_t i, CommonTaskHandle_t handle,std::future<R>& future) {
        auto itr=FreeIndex.find(i);
        if (itr == FreeIndex.end()) {
            return false;
        }
        FreeIndex.erase(i);
        UsedIndex.insert(i);
        Futures[i] = std::move(future);
        Handles[i] = handle;
        return true;
    }
    std::vector<FinishedSlotData_t>& CheckFinished() {
        OutData.clear();
        for (auto& id : UsedIndex) {
            if (Futures[id].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                OutData.push_back({ id,Handles[id] });
            }
        }
        for (auto& [id, handle] : OutData) {
            FreeIndex.insert(id);
            UsedIndex.erase(id);
        }
        return OutData;
    }
    std::vector<FinishedSlotData_t> OutData;
    std::set<uint8_t> FreeIndex;
    std::set<uint8_t> UsedIndex;
    std::vector<std::future<R>> Futures;
    std::vector<CommonTaskHandle_t> Handles;
};
