#pragma once

#include <functional>
#include <unordered_map>
#include "handle.h"


//template<typename ReturnType, typename ... ParamTypes>
//class TMulticastDelegate {
//    typedef std::function<ReturnType(ParamTypes...)> DelegateType;
//    std::unordered_map<CommonHandle_t, DelegateType>* pDelegatesMap;
//    TMulticastDelegate() {
//        pDelegatesMap
//    }
//};

#define DEFINE_DELEGATE_BASE(DelegateName)  \
    std::unordered_map<CommonHandle_t, DelegateName##DelegateType> DelegateName##Delegates; \
    virtual CommonHandle_t Add##DelegateName##Delegate(DelegateName##DelegateType indelegate) { \
        const auto& pair= DelegateName##Delegates.emplace(CommonHandle_t(CommonHandle_t::atomic_count), indelegate); \
        if (!pair.second) { \
            return CommonHandle_t(); \
        } \
        return pair.first->first; \
    } \
    virtual void Clear##DelegateName##Delegate(CommonHandle_t& handle) \
    { \
        DelegateName##Delegates.erase(handle); \
    } \
    virtual void Clear##DelegateName##Delegates() \
    { \
        DelegateName##Delegates.clear(); \
    }

#define DEFINE_EVENT(EventName)  \
    typedef std::function<void()> EventName##DelegateType; \
    DEFINE_DELEGATE_BASE(EventName) \
    virtual void Trigger##EventName##Delegates() \
    { \
        for (auto& pair : EventName##Delegates) { \
            pair.second(); \
        } \
    }


#define DEFINE_EVENT_ONE_PARAM(EventName, Param1Type)  \
    typedef std::function<void(Param1Type)> EventName##DelegateType; \
    DEFINE_DELEGATE_BASE(EventName) \
    virtual void Trigger##EventName##Delegates(Param1Type Param1) \
    { \
        for (auto& pair : EventName##Delegates) { \
            pair.second(Param1); \
        } \
    }

#define DEFINE_EVENT_TWO_PARAM(EventName, Param1Type, Param2Type)  \
    typedef std::function<void(Param1Type,Param2Type)> EventName##DelegateType; \
    DEFINE_DELEGATE_BASE(EventName) \
    virtual void Trigger##EventName##Delegates(Param1Type Param1,Param2Type Param2) \
    { \
        for (auto& pair : EventName##Delegates) { \
            pair.second(Param1,Param2); \
        } \
    }

#define DEFINE_EVENT_THREE_PARAM(EventName, Param1Type, Param2Type,Param3Type)  \
    typedef std::function<void(Param1Type,Param2Type,Param3Type)> EventName##DelegateType; \
    DEFINE_DELEGATE_BASE(EventName) \
    virtual void Trigger##EventName##Delegates(Param1Type Param1,Param2Type Param2,Param3Type Param3) \
    { \
        for (auto& pair : EventName##Delegates) { \
            pair.second(Param1,Param2,Param3); \
        } \
    }



#define DEFINE_DELEGATE_ONE_PARAM(EventName, Param1Type)  \
public: \
    typedef std::function<void(Param1Type)> EventName##DelegateType; \
private: \
    DEFINE_DELEGATE_BASE(EventName) \
    virtual void Trigger##EventName##Delegates(Param1Type Param1) \
    { \
        for (auto& pair : EventName##Delegates) { \
            pair.second(Param1); \
        } \
    } \
public:

#define DEFINE_DELEGATE_TWO_PARAM(EventName, Param1Type, Param2Type)  \
public: \
    typedef std::function<void(Param1Type,Param2Type)> EventName##DelegateType; \
private: \
    DEFINE_DELEGATE_BASE(EventName) \
    virtual void Trigger##EventName##Delegates(Param1Type Param1,Param2Type Param2) \
    { \
        for (auto& pair : EventName##Delegates) { \
            pair.second(Param1,Param2); \
        } \
    } \
public:
