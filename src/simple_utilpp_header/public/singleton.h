#pragma once
#include <memory>
template <typename T>
struct TClassSingletonHelper {
    static inline std::atomic<std::shared_ptr<T>> AtomicPtr;

    template <typename... Args>
    static std::shared_ptr<T> GetClassSingletonByConstructor(std::function<T* (Args...)> func, Args&&... args) {
        auto oldptr = AtomicPtr.load();
        if (!oldptr) {
            std::shared_ptr<T> ptr(func(args...));
            AtomicPtr.compare_exchange_strong(oldptr, ptr);
        }
        return AtomicPtr.load();
    }

    template <typename... Args>
    static std::shared_ptr<T> GetClassSingleton(Args&&... args) {
        auto oldptr = AtomicPtr.load();
        if (!oldptr) {
            std::shared_ptr<T> ptr(new T(args...));
            AtomicPtr.compare_exchange_strong(oldptr, ptr);
        }
        return AtomicPtr.load();
    }
};

//template <typename T, typename... Args>
//std::shared_ptr<T> GetClassSingleton(std::function<T*(Args...)> func,Args&&... args) {
//    static std::atomic<std::shared_ptr<T>> AtomicPtr;
//    auto oldptr = AtomicPtr.load();
//    if (!oldptr) {
//        std::shared_ptr<T> ptr(func(args...));
//        AtomicPtr.compare_exchange_strong(oldptr, ptr);
//    }
//    return AtomicPtr.load();
//}
//
//template <typename T, typename... Args>
//std::shared_ptr<T> GetClassSingleton(Args&&... args) {
//    static std::atomic<std::shared_ptr<T>> AtomicPtr;
//    auto oldptr = AtomicPtr.load();
//    if (!oldptr) {
//        std::shared_ptr<T> ptr=std::make_shared<T>(args...);
//        AtomicPtr.compare_exchange_strong(oldptr, ptr);
//    }
//    return AtomicPtr.load();
//}