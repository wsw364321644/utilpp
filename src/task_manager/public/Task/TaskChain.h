#pragma once
#include "Task/TaskManager.h"
#include "task_manager_export_defs.h"

#include <singleton.h>
#include <future>
#include <queue>
#include <unordered_map>
#include <memory>

struct TaskChain_t;
class ITaskChainNodeBase {
public:
    void SetWorkflow(WorkflowHandle_t workflowHandle) {
        WorkflowHandle = workflowHandle;
    }
    void SetOwner(TaskChain_t* owner) {
        Owner = owner;
    }
    virtual void Post() = 0;
    WorkflowHandle_t WorkflowHandle;
    TaskChain_t* Owner;
};

class IFutureWrapperBase {
public:
    virtual bool IsComplete() = 0;
};

template <class R>
class FFutureWrapper :public IFutureWrapperBase {
public:
    bool IsComplete() {
        return Future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
    std::future<R> Future;
};



typedef struct TaskChain_t {
    std::queue<std::shared_ptr<ITaskChainNodeBase>> TaskChainNodes;
    std::shared_ptr<IFutureWrapperBase> pFuture;
    CommonTaskHandle_t TaskHandle;
}TaskChain_t;


template <typename F, typename R = std::invoke_result_t<std::decay_t<F>>, std::enable_if_t<!std::is_invocable_v<std::decay_t<F>, std::shared_ptr<std::promise<R>>>, int> = 0>
class FTaskChainNode :public ITaskChainNodeBase,
    public std::enable_shared_from_this<FTaskChainNode<F, R>> {
public:
    FTaskChainNode(F&& func, std::function<void()> onComplete) :Func(std::forward<F>(func)),OnComplete(std::move(onComplete)) {}
    void Post() {
        auto self= this->shared_from_this();
        auto [hTask, future] = GetTaskManagerSingleton()->AddTask(WorkflowHandle,
            [this, self]() {
                if constexpr (std::is_void_v<R>) {
                    Func();
                }
                else {
                    Ret = Func();
                }
                if (WorkflowHandle == GetTaskManagerSingleton()->GetMainThread()) {
                    OnComplete();
                }
                else {
                    GetTaskManagerSingleton()->AddTask(GetTaskManagerSingleton()->GetMainThread(),
                        [this, self]() {
                            OnComplete();
                        }
                    );
                }

            }
        );
        auto pFuture = std::make_shared<FFutureWrapper<R>>();
        pFuture->Future = std::move(future);
        Owner->pFuture = pFuture;
        Owner->TaskHandle = hTask;
    }
    F Func;
    std::function<void()> OnComplete;
    using StorageType = std::conditional_t<std::is_void_v<R>, std::monostate, R>;
    StorageType Ret{};
};
