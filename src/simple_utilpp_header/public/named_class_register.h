#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include "std_ext.h"

template<class T>
struct TNamedClassInfo_t {
    std::atomic<std::shared_ptr<T>> Ptr;
    std::function<std::shared_ptr<T>()> CreateFn;
    std::u8string Name;
};
template<class T>
class TNamedClassRegister {
public:
    typedef T RegisterClassParent;
    typedef TNamedClassRegister<RegisterClassParent> FNamedClassRegister;
    typedef TNamedClassInfo_t<T> NamedClassInfo_t;

    template<class RegisterClassChild>
    static void RigisterNamedClass(std::u8string_view name) {
        auto pNamedClassInfo = std::make_shared<NamedClassInfo_t>();
        pNamedClassInfo->Name = name;

        auto [pair, res] = NamedClassInfos.try_emplace(pNamedClassInfo->Name, pNamedClassInfo);
        auto& [key, val] = *pair;
        pNamedClassInfo->CreateFn =
            []() ->std::shared_ptr<RegisterClassParent> {
            return std::make_shared<RegisterClassChild>();
            };
    };
    static std::shared_ptr<RegisterClassParent> GetNamedClass(const char* name) {
        return GetNamedClass((const char8_t*)name);
    }
    static std::shared_ptr<RegisterClassParent> GetNamedClass(std::u8string_view name) {
        auto itr = NamedClassInfos.find(name);
        if (itr == NamedClassInfos.end()) {
            return nullptr;
        }
        auto& [_name, pNamedClassInfo] = *itr;
        auto& namedClassInfo = *pNamedClassInfo;
        auto expected = namedClassInfo.Ptr.load();
        if (expected) {
            return expected;
        }
        auto desired = namedClassInfo.CreateFn();
        namedClassInfo.Ptr.compare_exchange_strong(expected, desired);
        return namedClassInfo.Ptr.load();
    }
    inline static std::unordered_map<std::u8string_view, std::shared_ptr<NamedClassInfo_t>, string_hash> NamedClassInfos;
};

template <typename T>
class MyClass {
public:
    template <typename U>
    static U myStaticFunction(U value) {
        return value;
    }
};

template<class T>
struct TNamedClassAutoRegister_t {
    TNamedClassAutoRegister_t(std::u8string_view name) {
        T::template RigisterNamedClass<T>(name);
    }
    TNamedClassAutoRegister_t(const char* name) {
        TNamedClassAutoRegister_t((const char8_t*)name);
    }
};
