#include "SimpleValueStorage.h"
#include "simple_value_storage.h"
#include <atomic>
#include <list>
#include <memory>
#include <cstring>
#define INCREASE_SIZE 64
std::atomic<SimpleValueHandle_t::CommonHandleID_t> SimpleValueHandle_t::SimpleValueCount{ 0 };
typedef struct StorageValueInfo_t {
    std::atomic_intptr_t ValueCache{ 0 };
    std::atomic_intptr_t Value{ 0 };
    std::atomic_intptr_t LastestValue{ 0 };
    uint32_t Size{ 0 };
    ValueChangedDelegate_t  Delegate;
    ConstructFn_t LConstructFn;
    ConstructFn_t RConstructFn;
    DeconstructFn_t DeconstructFn;
}StorageValueInfo_t;

std::unordered_map<SimpleValueHandle_t,std::shared_ptr<StorageValueInfo_t>, std::hash<CommonHandle32_t>> StorageValueInfoList;
const SimpleValueHandle_t SimpleValueStorage::RegisterValue(uint32_t size, ConstructFn_t LConstructFn, ConstructFn_t RConstructFn, DeconstructFn_t deconstructfn)
{
    auto pinfo=std::make_shared<StorageValueInfo_t>();
    auto res=StorageValueInfoList.try_emplace(SimpleValueHandle_t(SimpleValueHandle_t::SimpleValueCount), pinfo);
    if (!res.second) {
        return NullHandle;
    }
    pinfo->Size = size;
    if (LConstructFn) {
        pinfo->LConstructFn = LConstructFn;
    }
    else {
        pinfo->LConstructFn =[size](void* dest, void* src) {
            memcpy(dest, src, size);
            };
    }
    if (RConstructFn) {
        pinfo->RConstructFn = RConstructFn;
    }
    else {
        pinfo->RConstructFn= pinfo->LConstructFn;
    }
    if (deconstructfn) {
        pinfo->DeconstructFn = deconstructfn;
    }
    else {
        pinfo->DeconstructFn = [](void* src) {
            free(src);
            };
    }

    return res.first->first;
}

bool SimpleValueStorage::RegisterValueChange(SimpleValueHandle_t handle, ValueChangedDelegate_t Delegate)
{
    auto res = StorageValueInfoList.find(handle);
    if (res == StorageValueInfoList.end()) {
        return false;
    }
    StorageValueInfo_t& info = *res->second;
    info.Delegate = Delegate;
    return true;
}

void SimpleValueStorage::RemoveValue(const SimpleValueHandle_t handle)
{
    StorageValueInfoList.erase(handle);
}

bool SimpleValueStorage::SetValue(SimpleValueHandle_t handle, void* value, bool lvalue)
{
    auto res=StorageValueInfoList.find(handle);
    if (res == StorageValueInfoList.end()) {
        return false;
    }
    StorageValueInfo_t& info=*res->second;
    auto buf = malloc(info.Size);
    if (!buf) {
        return false;
    }
    if (lvalue) {
        info.LConstructFn(buf, value);
    }
    else {
        info.RConstructFn(buf, value);
    }
    bool result;
    intptr_t previousNew;
    previousNew = info.LastestValue.load();
    do {
        result = info.LastestValue.compare_exchange_weak(previousNew, (intptr_t)buf);
    } while (!result);
    if (previousNew) {
        info.DeconstructFn((void*)previousNew);
    }
    return true;
}

bool SimpleValueStorage::GetValue(SimpleValueHandle_t handle, void* buf)
{
    intptr_t Expected{ 0 };
    auto res = StorageValueInfoList.find(handle);
    if (res == StorageValueInfoList.end()) {
        return false;
    }
    StorageValueInfo_t& info = *res->second;
    auto previousValue= info.Value.load();
    if (!previousValue) {
        return false;
    }
    if (!info.Value.compare_exchange_weak(previousValue, Expected)) {
        return false;
    }
    info.LConstructFn(buf, (void*)previousValue);
    if (!info.Value.compare_exchange_strong(Expected, previousValue)) {
        info.DeconstructFn((void*)previousValue);
    }
    return true;
}

void SimpleValueStorage::Tick(float delta)
{
    for (auto& pair : StorageValueInfoList) {
        StorageValueInfo_t& info = *pair.second;
        auto previousValue = info.Value.load();
        auto previousLastestValue = info.LastestValue.load();
        intptr_t ExpectedLastestValue{ 0 };
        intptr_t ExpectedValue{ 0 };
        if (!previousLastestValue) {
            continue;
        }
        if (previousLastestValue == previousValue) {
            continue;
        }
        if (!info.LastestValue.compare_exchange_weak(previousLastestValue, ExpectedLastestValue)) {
            continue;
        }
        if (!info.Value.compare_exchange_weak(previousValue, previousLastestValue)) {
            if (!info.LastestValue.compare_exchange_strong(ExpectedLastestValue, previousLastestValue)) {
                info.DeconstructFn((void*)previousLastestValue);
            }
            continue;
        }
        info.Delegate(pair.first, (void*)previousLastestValue, (void*)previousValue);
        if (previousValue) {
            info.DeconstructFn((void*)previousValue);
        }
    }
}
