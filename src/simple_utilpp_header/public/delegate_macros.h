#pragma once

#include <functional>
#include <unordered_map>
#include "handle.h"


#define DEFINE_DELEGATE_BASE(DelegateName)  \
    std::unordered_map<CommonHandle32_t, DelegateName##DelegateType> DelegateName##Delegates; \
    CommonHandle32_t Add##DelegateName##Delegate(DelegateName##DelegateType indelegate) { \
        const auto& pair= DelegateName##Delegates.emplace(CommonHandle32_t(CommonHandle32_t::atomic_count), indelegate); \
        if (!pair.second) { \
            return NullHandle; \
        } \
        return pair.first->first; \
    } \
    void Clear##DelegateName##Delegate(CommonHandle32_t& handle) \
    { \
        DelegateName##Delegates.erase(handle); \
    } \
    void Clear##DelegateName##Delegates() \
    { \
        DelegateName##Delegates.clear(); \
    }

#define DEFINE_EVENT(EventName)  \
    typedef std::function<void()> EventName##DelegateType; \
    DEFINE_DELEGATE_BASE(EventName) \
    void Trigger##EventName##Delegates() \
    { \
        std::vector<CommonHandle32_t> keys;\
        keys.reserve(EventName##Delegates.size()); \
        for (const auto& [key, _] : EventName##Delegates) {\
            keys.push_back(key);\
        }\
        for (auto& key :keys) { \
            auto itr=EventName##Delegates.find(key);\
            if(itr==EventName##Delegates.end()){\
                continue;\
            }\
            itr->second(); \
        } \
    }

#define DEFINE_EVENT_ONE_PARAM(EventName, Param1Type)  \
    typedef std::function<void(Param1Type)> EventName##DelegateType; \
    DEFINE_DELEGATE_BASE(EventName) \
    void Trigger##EventName##Delegates(Param1Type Param1) \
    { \
        std::vector<CommonHandle32_t> keys;\
        keys.reserve(EventName##Delegates.size()); \
        for (const auto& [key, _] : EventName##Delegates) {\
            keys.push_back(key);\
        }\
        for (auto& key :keys) { \
            auto itr=EventName##Delegates.find(key);\
            if(itr==EventName##Delegates.end()){\
                continue;\
            }\
            itr->second(Param1); \
        } \
    }

#define DEFINE_EVENT_TWO_PARAM(EventName, Param1Type, Param2Type)  \
    typedef std::function<void(Param1Type,Param2Type)> EventName##DelegateType; \
    DEFINE_DELEGATE_BASE(EventName) \
    void Trigger##EventName##Delegates(Param1Type Param1,Param2Type Param2) \
    { \
        std::vector<CommonHandle32_t> keys;\
        keys.reserve(EventName##Delegates.size()); \
        for (const auto& [key, _] : EventName##Delegates) {\
            keys.push_back(key);\
        }\
        for (auto& key :keys) { \
            auto itr=EventName##Delegates.find(key);\
            if(itr==EventName##Delegates.end()){\
                continue;\
            }\
            itr->second(Param1,Param2); \
        } \
    }

#define DEFINE_EVENT_THREE_PARAM(EventName, Param1Type, Param2Type,Param3Type)  \
    typedef std::function<void(Param1Type,Param2Type,Param3Type)> EventName##DelegateType; \
    DEFINE_DELEGATE_BASE(EventName) \
    void Trigger##EventName##Delegates(Param1Type Param1,Param2Type Param2,Param3Type Param3) \
    { \
        std::vector<CommonHandle32_t> keys;\
        keys.reserve(EventName##Delegates.size()); \
        for (const auto& [key, _] : EventName##Delegates) {\
            keys.push_back(key);\
        }\
        for (auto& key :keys) { \
            auto itr=EventName##Delegates.find(key);\
            if(itr==EventName##Delegates.end()){\
                continue;\
            }\
            itr->second(Param1,Param2,Param3); \
        } \
    }

