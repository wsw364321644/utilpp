#pragma once

#include <algorithm>
#include <type_traits>
#include <memory>
#include "simple_export_ppdefs.h"

/// @details ThroughCRTWrapper help free value use same crt
/// https://learn.microsoft.com/en-us/cpp/c-runtime-library/potential-errors-passing-crt-objects-across-dll-boundaries
/// Be careful with ThroughCRTWrapper wrap a template class or ThroughCRTWrapper wrap a type come from a static library. 
/// Triggering their dynamic alloc memory is an error.
template<class value_type>
class ThroughCRTWrapper {
public:
    typedef void(*DeconstructFn_t)(value_type*);
    ThroughCRTWrapper() {}
    ThroughCRTWrapper(ThroughCRTWrapper&) = delete;
    ThroughCRTWrapper(ThroughCRTWrapper&& other) {
        *this = std::forward<ThroughCRTWrapper&&>(other);
    }
    template <class... _Types>
    ThroughCRTWrapper(_Types&&... _Args) {
        SetValue(std::forward<_Types>(_Args)...);
    }
    ~ThroughCRTWrapper() {
        Reset();
    }

    ThroughCRTWrapper& operator =(ThroughCRTWrapper&& other) {
        std::swap(freeFunc, other.freeFunc);
        std::swap(Value, other.Value);
        return *this;
    }
    ThroughCRTWrapper& operator=(value_type&& value) {
        SetValue(std::forward<value_type>(value));
    }

    template <class... _Types>
    void SetValue(_Types&&... _Args) {
        Reset();
        Value = new value_type(std::forward<_Types>(_Args)...);
        freeFunc = [](value_type* value) {
            if (value) {
                delete value;
            }
            };
    }

    void Reset() {
        if (freeFunc) {
            freeFunc(Value);
            freeFunc = nullptr;
            Value = nullptr;
        }
    }
    value_type& GetValue() const {
        return *Value;
    }

private:
    DeconstructFn_t freeFunc{ nullptr };
    value_type* Value{ nullptr };
};



//template<class value_type, std::enable_if_t<std::is_pointer<value_type>::value, bool> = true>

template<class T>
class ThroughCRTWrapper<std::shared_ptr<T>> {
public:
    typedef void(*DeconstructFn_t)(std::shared_ptr<T>);
    using test_type = ThroughCRTWrapper;
    ThroughCRTWrapper() {}
    ThroughCRTWrapper(std::nullptr_t ptr) {
    }

    ThroughCRTWrapper(ThroughCRTWrapper&& other) {
        *this = std::forward<ThroughCRTWrapper>(other);
    }
    ThroughCRTWrapper(const ThroughCRTWrapper& other) {
        *this = other;
    }
    ThroughCRTWrapper(std::shared_ptr<T> && ptr) {
        SetValue(std::forward<std::shared_ptr<T>>(ptr));
    }
    ThroughCRTWrapper(const std::shared_ptr<T> & ptr) {
        SetValue(ptr);
    }
    ~ThroughCRTWrapper() {
        Reset();
    }

    ThroughCRTWrapper& operator =(const ThroughCRTWrapper& other) {
        freeFunc= other.freeFunc;
        Value= other.Value;
        return *this;
    }
    ThroughCRTWrapper& operator =(ThroughCRTWrapper&& other) {
        std::swap(freeFunc, other.freeFunc);
        std::swap(Value, other.Value);
        return *this;
    }
    template<class _T , std::enable_if_t<std::is_same_v<std::decay_t<_T>, std::shared_ptr<T>>,int> = 0>
    ThroughCRTWrapper& operator=(const _T&& value) {
        SetValue(std::forward<std::shared_ptr<T>>(value));
    }

    template<class _T, std::enable_if_t<std::is_same_v<std::decay_t<_T>, std::shared_ptr<T>>, int> = 0>
    void SetValue(_T && ptr) {
        Reset();
        Value = ptr;
        freeFunc = [](std::shared_ptr<T> value) {
            value.reset();
            };
    }

    void Reset() {
        if (freeFunc) {
            freeFunc(std::move(Value));
            freeFunc = nullptr;
            Value = nullptr;
        }
    }
    T* GetValue() const {
        return Value.get();
    }
private:
    DeconstructFn_t freeFunc{ nullptr };
    std::shared_ptr<T> Value{ nullptr };
};


template<class T>
class ThroughCRTWrapper<std::weak_ptr<T>> {
public:
    typedef void(*DeconstructFn_t)(std::weak_ptr<T>);
    ThroughCRTWrapper() {}
    ThroughCRTWrapper(ThroughCRTWrapper&) = delete;
    ThroughCRTWrapper(ThroughCRTWrapper&& other) {
        *this = std::forward<ThroughCRTWrapper&&>(other);
    }
    template<class _T = std::weak_ptr<T>>
    ThroughCRTWrapper(_T&& ptr) {
        SetValue(std::forward<std::weak_ptr<T>>(ptr));
    }
    ~ThroughCRTWrapper() {
        Reset();
    }


    ThroughCRTWrapper& operator =(ThroughCRTWrapper&& other) {
        std::swap(freeFunc, other.freeFunc);
        std::swap(Value, other.Value);
        return *this;
    }
    template<class _T = std::weak_ptr<T>>
    ThroughCRTWrapper& operator=(_T&& value) {
        SetValue(std::forward<std::weak_ptr<T>>(value));
    }

    void SetValue(std::weak_ptr<T>&& ptr) {
        Reset();
        Value = ptr;
        freeFunc = [](std::shared_ptr<T> value) {
            value.reset();
            };
    }

    void Reset() {
        if (freeFunc) {
            freeFunc(std::move(Value));
            freeFunc = nullptr;
            Value = nullptr;
        }
    }
    const T* GetValue() const {
        auto pValue= Value.lock();
        return pValue.get();
    }
private:
    DeconstructFn_t freeFunc{ nullptr };
    std::weak_ptr<T> Value{ nullptr };
};


#include <string>
SIMPLE_UTIL_EXPORT ThroughCRTWrapper<std::shared_ptr<std::string>> TestGetString();