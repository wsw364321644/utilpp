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

#include <delegate_macros.h>
#include "task_manager_export_defs.h"
constexpr std::chrono::nanoseconds DEFAULT_REPEAT_TIME = (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) / 60);

class  ITaskManager;

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

typedef std::function<void()> TCommonTask;
typedef std::function<void(CommonTaskHandle_t)> TTimerTask;
typedef std::function<void(float)> TTickTask;

class TASK_MANAGER_EXPORT FCancelableTaskBase {

public:
    enum ERunningStatus {
        Null,
        Running,
        Finished,
    };
    virtual ~FCancelableTaskBase() = 0;
    /**
    * task will tick after AddCancelableTask.
    */
    virtual void Tick(float delta) = 0;
    /**
    * task will notice OnStop after RemoveTask.
    */
    virtual void OnStop()=0;
    /**
    * task last run Finalize.
    */
    virtual void Finalize();
    /**
    * reset clean  FCancelableTaskBase for another AddCancelableTask Call
    */
    virtual void Reset();
    /**
    * run once when add
    */
    virtual void Init(CommonTaskHandle_t);
    /**
    * set Finalize run which workflow.
    * Finalize run same thread with tick when FinalizeWorkflow not set.
    */
    void SetFinalizeWorkflow(WorkflowHandle_t Workflow);
    //DEFINE_EVENT(TaskStop)

    ERunningStatus RunningStatus{ ERunningStatus ::Null};
    WorkflowHandle_t FinalizeWorkflow{NullHandle};
    CommonTaskHandle_t SelfHandle{NullHandle};
    ITaskManager* Manager{nullptr};
};

//class TASK_MANAGER_EXPORT FCancelableTaskTempWrapper:public FCancelableTaskBase {
//public:
//    typedef FCancelableTaskBase super;
//    void Tick(float delta) override;
//    void Init() override;
//    void Finalize() override;
//    std::function<void()> ReleaseFunc;
//    FCancelableTaskBase* Task{ nullptr };
//};

class TASK_MANAGER_EXPORT FCancelableTaskTemp :public FCancelableTaskBase {
public:
    typedef FCancelableTaskBase super;

    void Finalize() override;
    void SetReleaseFunc(const std::function<void()>);
    std::function<void()> ReleaseFunc;
};

class  ITaskManager {
public:
    virtual WorkflowHandle_t NewWorkflow() = 0;
    /**
    * get MainThread handle
    */
    virtual WorkflowHandle_t GetMainThread() = 0;
    virtual void ReleaseWorkflow(WorkflowHandle_t) = 0;
    virtual void Run() = 0;
    virtual void Tick() = 0;
    virtual void Stop() = 0;
    //template <typename _Ty, class _Alloc = std::allocator<std::pair<FCancelableTaskTempWrapper, _Ty>>, typename E = std::enable_if_t<std::is_base_of_v< FCancelableTaskBase, _Ty>, void>>
    //std::tuple<FCancelableTaskTempWrapper*, _Ty*> CreateCancelableTaskTempWrapperByClass() {
    //    _Alloc alloc;
    //    std::pair<FCancelableTaskTempWrapper, _Ty>* pair= alloc.allocate(1);
    //    //std::pair<FCancelableTaskTempWrapper, _Ty> test{};
    //    new (&pair->first)FCancelableTaskTempWrapper();
    //    new (&pair->second)_Ty();
    //    pair->first.Task = &pair->second;
    //    pair->first.ReleaseFunc = [pair, alloc]()mutable {
    //        pair->first.~FCancelableTaskTempWrapper();
    //        pair->second.~_Ty();
    //        alloc.deallocate(pair, 1);
    //        };
    //    return { &pair->first  ,&pair->second };
    //}
    virtual CommonTaskHandle_t AddCancelableTask(WorkflowHandle_t handle, FCancelableTaskBase* task) = 0;
    virtual CommonTaskHandle_t AddTick(WorkflowHandle_t, TTickTask task) = 0;
    virtual CommonTaskHandle_t AddTimer(WorkflowHandle_t, TTimerTask task, uint64_t repeat, uint64_t timeout = 0) = 0;
    virtual CommonTaskHandle_t AddTaskNoReturn(WorkflowHandle_t handle, TCommonTask task) = 0;
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