#pragma once
#include <memory>
#include <functional>
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

template<class T>
class TProvideSingletonClass {
public:
    static std::shared_ptr<T> GetSingleton() {
        return TClassSingletonHelper<T>::GetClassSingleton();
    }
};

//template <typename T>
//struct TClassThreadSingletonHelper {
//    inline thread_local static std::atomic<std::shared_ptr<T>> AtomicPtr;
//
//    template <typename... Args>
//    static std::shared_ptr<T> GetClassSingletonByConstructor(std::function<T* (Args...)> func, Args&&... args) {
//        auto oldptr = AtomicPtr.load();
//        if (!oldptr) {
//            std::shared_ptr<T> ptr(func(args...));
//            AtomicPtr.compare_exchange_strong(oldptr, ptr);
//        }
//        return AtomicPtr.load();
//    }
//
//    template <typename... Args>
//    static std::shared_ptr<T> GetClassSingleton(Args&&... args) {
//        auto oldptr = AtomicPtr.load();
//        if (!oldptr) {
//            std::shared_ptr<T> ptr(new T(args...));
//            AtomicPtr.compare_exchange_strong(oldptr, ptr);
//        }
//        return AtomicPtr.load();
//    }
//};

template<class T>
class TProvideThreadSingletonClass {
public:
    inline thread_local static std::shared_ptr<T> Instance;
    static std::shared_ptr<T> GetThreadSingleton() {
        if (!Instance.get()) {
            Instance = std::make_shared<T>();
        }
        return Instance;
    }
};
