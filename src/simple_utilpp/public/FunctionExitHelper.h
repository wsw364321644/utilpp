#pragma once

#include <functional>

typedef struct FunctionExitHelper_t {
    FunctionExitHelper_t(std::function<void()> infunc) :func(infunc) {}
    ~FunctionExitHelper_t() {
        func();
    }
    std::function<void()> func;
}FunctionExitHelper_t;