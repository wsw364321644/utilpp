#pragma once
#include <functional>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <mutex>
#include <atomic>
#include <future>
#include <tuple>
#include <set>
#include <optional>
#include <shared_mutex>
#include <handle.h>
#include <TimeRecorder.h>
#include "task_manager_export_defs.h"
constexpr std::chrono::nanoseconds DEFAULT_REPEAT_TIME = (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) / 60);




typedef struct TASK_MANAGER_EXPORT CommonTaskHandle_t : CommonHandle_t
{
    CommonTaskHandle_t(CommonHandle_t h) :CommonHandle_t(h) {}
    CommonTaskHandle_t() :CommonHandle_t(TaskCount) {}
    CommonTaskHandle_t(const NullCommonHandle_t handle) :CommonHandle_t(handle) {}
    static std::atomic_uint32_t TaskCount;
}CommonTaskHandle_t;

typedef struct TASK_MANAGER_EXPORT WorkflowHandle_t : CommonHandle_t
{
    WorkflowHandle_t(NullCommonHandle_t h) :CommonHandle_t(h) {}
    WorkflowHandle_t(CommonHandle_t h) :CommonHandle_t(h) {}
    WorkflowHandle_t() : CommonHandle_t(WorkflowCount) {}
    static std::atomic_uint32_t WorkflowCount;
}WorkflowHandle_t;


typedef std::function<void()> FCommonTask;
typedef std::function<void(CommonTaskHandle_t)> FTimerTask;
typedef std::function<void(float)> FTickTask;


class  ITaskManager {
public:
    virtual WorkflowHandle_t NewWorkflow() = 0;
    // get MainThread handle
    virtual WorkflowHandle_t GetMainThread() = 0;
    virtual void ReleaseWorkflow(WorkflowHandle_t) = 0;
    virtual void Run() = 0;
    virtual void Tick() = 0;
    virtual void Stop() = 0;

    virtual CommonTaskHandle_t AddTick(WorkflowHandle_t, FTickTask task) = 0;
    virtual CommonTaskHandle_t AddTimer(WorkflowHandle_t, FTimerTask task, uint64_t repeat, uint64_t timeout = 0) = 0;
    virtual CommonTaskHandle_t AddTaskNoReturn(WorkflowHandle_t handle, FCommonTask task) = 0;
    // pass a task to excuse
    template <typename F, typename R = std::invoke_result_t<std::decay_t<F>>>
    std::tuple<CommonTaskHandle_t, std::future<R>> AddTask(WorkflowHandle_t WorkflowHandle, F&& task) {
        const std::shared_ptr<std::promise<R>> task_promise = std::make_shared<std::promise<R>>();
        auto handle = AddTaskNoReturn(WorkflowHandle,
            [task = std::forward<F>(task), task_promise]() {
                try
                {
                    if constexpr (std::is_void_v<R>)
                    {
                        task();
                        task_promise->set_value();
                    }
                    else
                    {
                        task_promise->set_value(task());
                    }
                }
                catch (...)
                {
                    try
                    {
                        task_promise->set_exception(std::current_exception());
                    }
                    catch (...)
                    {
                    }
                }
            }
        );
        return { handle, task_promise->get_future() };
    }
    virtual void RemoveTask(CommonTaskHandle_t) = 0;
};


TASK_MANAGER_EXPORT ITaskManager* GetTaskManagerInstance();