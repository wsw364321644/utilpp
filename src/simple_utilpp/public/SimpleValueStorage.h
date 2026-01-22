#pragma once

#include <handle.h>
#include <string_view>
#include <functional>
#include <type_traits>
#include "simple_export_ppdefs.h"
typedef struct SimpleValueHandle_t : CommonHandle32_t
{
    constexpr SimpleValueHandle_t(NullCommonHandle_t nhandle) :CommonHandle32_t(nhandle) { }
    SimpleValueHandle_t() : CommonHandle32_t() {}
    constexpr SimpleValueHandle_t(CommonHandleID_t id) : CommonHandle32_t(id) {}
    constexpr SimpleValueHandle_t(const SimpleValueHandle_t& handle) : CommonHandle32_t(handle) {}

    virtual ~SimpleValueHandle_t() {}
    static std::atomic<CommonHandleID_t> SimpleValueCount;
}SimpleValueHandle_t;

typedef std::function<void(SimpleValueHandle_t,const void*, const void*)> ValueChangedDelegate_t;

typedef std::function<void(void*,void*)> ConstructFn_t;
typedef std::function<void(void*)> DeconstructFn_t;

class SIMPLE_UTIL_EXPORT SimpleValueStorage {
public:
    template<class value_type>
    static SimpleValueHandle_t RegisterValue() {
        return RegisterValue(sizeof(value_type), 
            [](void* dst, void* src) {
                new(dst)value_type(*(value_type*)src);
            },
            [](void* dst, void* src) {
                new(dst)value_type(std::move(*(value_type*)src));
            },
            [](void* src) {
                delete((value_type*)src);
            }
            );
    }

    template<class value_type>
    static bool SetValue(SimpleValueHandle_t handle,value_type&& value) {
        return SetValue(handle, &value, std::is_lvalue_reference_v<value_type>);
    }

    template<class value_type>
    static bool GetValue(SimpleValueHandle_t handle, value_type& value) {
        return GetValue(handle,&value);
    }
    /// @brief register a value in storage
    /// @details thread not safe
    /// @return value handle
    /// @param[in] size  value size
    static const SimpleValueHandle_t RegisterValue(uint32_t size, ConstructFn_t lConstructfn = nullptr, ConstructFn_t rConstructfn = nullptr, DeconstructFn_t deconstructfn = nullptr);
    static bool RegisterValueChange(SimpleValueHandle_t handle, ValueChangedDelegate_t Delegate);
    static void RemoveValue(const SimpleValueHandle_t handle);
    static bool SetValue(SimpleValueHandle_t,void*,bool lvalue=true);
    static bool GetValue(SimpleValueHandle_t,void*);
    static void Tick(float delta);

    //static SimpleValueHandle_t RegisterValueWithName(std::string_view name, uint32_t size);
    //static bool SetValueByName(std::string_view name);
    //static bool GetValueByName(std::string_view name);
};