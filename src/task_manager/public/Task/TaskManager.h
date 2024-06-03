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

typedef struct TimerTaskData_t {
    WorkflowHandle_t Workflow;
    FTimerTask Task;
    std::chrono::nanoseconds Repeat;
    std::chrono::nanoseconds Timeout;
}TimerTaskData_t;

typedef struct CommonTaskData_t {
    WorkflowHandle_t Workflow;
    FCommonTask Task;
}CommonTaskData_t;

typedef struct TickTaskData_t {
    WorkflowHandle_t Workflow;
    FTickTask Task;
}TickTaskData_t;

typedef struct TaskWorkflow_t {
    std::chrono::nanoseconds RepeatTime{ DEFAULT_REPEAT_TIME };
    std::chrono::nanoseconds Timeout{ 0 };
    FTimeRecorder TimeRecorder;
    std::optional<std::future<void>> OptFuture;
    std::vector<CommonTaskHandle_t> TimerTasks;
    std::vector<CommonTaskHandle_t> TickTasks;
    std::set<CommonTaskHandle_t> Tasks;
}TaskWorkflow_t;

class TASK_MANAGER_EXPORT FTaskManager {
public:
    static FTaskManager* Get();
    ~FTaskManager();

    WorkflowHandle_t NewWorkflow();
    // get MainThread handle
    WorkflowHandle_t GetMainThread();
    void ReleaseWorkflow(WorkflowHandle_t);
    void Run();
    void Tick();
    
    void Stop();

    CommonTaskHandle_t AddTick(WorkflowHandle_t, FTickTask task);
    CommonTaskHandle_t AddTimer(WorkflowHandle_t, FTimerTask task, uint64_t repeat, uint64_t timeout = 0);
    // pass a task to excuse
    template <typename F, typename R = std::invoke_result_t<std::decay_t<F>>>
    std::tuple<CommonTaskHandle_t,std::future<R>> AddTask(WorkflowHandle_t handle, F&& task) {
        const std::shared_ptr<std::promise<R>> task_promise = std::make_shared<std::promise<R>>();
        std::shared_lock TaskWorkflowRLock{ TaskWorkflowMutex, std::defer_lock };
        std::unique_lock TaskLock{ TaskMutex, std::defer_lock };
        std::scoped_lock lock(TaskWorkflowRLock, TaskLock);
        auto itr = TaskWorkflowDatas.find(handle);
        if (itr == TaskWorkflowDatas.end()) {
            return { NullHandle, task_promise->get_future() };
        }
        auto res = Tasks.emplace(CommonTaskHandle_t::TaskCount, std::make_shared < CommonTaskData_t>(handle, 
            [task = std::forward<F>(task),task_promise]() {
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
        ));
        if (!res.second) {
            return { NullHandle, task_promise->get_future() };
        }
        auto setres = itr->second->Tasks.emplace(res.first->first);
        if (!setres.second) {
            Tasks.erase(res.first->first);
            return { NullHandle, task_promise->get_future() };
        }
        return { res.first->first, task_promise->get_future() };

    }
    void RemoveTask(CommonTaskHandle_t);

private:
    void TickTaskWorkflow(WorkflowHandle_t handle);
    void TickRandonTaskWorkflow();
    FTaskManager();

    std::unordered_map<CommonHandle_t, std::shared_ptr<TaskWorkflow_t>>  TaskWorkflowDatas;
    std::shared_mutex  TaskWorkflowMutex;

    std::unordered_map < CommonHandle_t, std::shared_ptr<TickTaskData_t>> TickTasks;
    std::unordered_map < CommonHandle_t, std::shared_ptr<TimerTaskData_t>> TimerTasks;
    std::unordered_map < CommonHandle_t, std::shared_ptr<CommonTaskData_t>> Tasks;
    std::shared_mutex  TaskMutex;
    std::shared_mutex  TickMutex;
    std::shared_mutex  TimerMutex;

    void* InterData;
    std::atomic_bool bRequestExit{ false };
    WorkflowHandle_t MainThread;
};
