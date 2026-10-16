#pragma once
#include "Task/TaskManager.h"
#include "task_manager_export_defs.h"

#include <singleton.h>
#include <future>
#include <queue>
#include <unordered_map>
#include <memory>
#include <any>
#include <optional>

struct TaskChain_t;
class ITaskChainNodeBase {
public:
    virtual ~ITaskChainNodeBase() = default;
    void SetWorkflow(WorkflowHandle_t workflowHandle) {
        WorkflowHandle = workflowHandle;
    }
    void SetOwner(TaskChain_t* owner) {
        Owner = owner;
    }
    virtual void Post() = 0;

    virtual void ExtractResultToContext(std::any& context) = 0;
    virtual void InjectInputFromContext(std::any& context) = 0;

    WorkflowHandle_t WorkflowHandle;
    TaskChain_t* Owner;
};

class IFutureWrapperBase {
public:
    virtual ~IFutureWrapperBase() = default;
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
    std::any DataContext;
}TaskChain_t;


template <typename PrevR, typename F>
class FTaskChainNode :public ITaskChainNodeBase,
    public std::enable_shared_from_this<FTaskChainNode<PrevR,F>> {
public:
    using R = SafeInvokeResult_t<F, PrevR>;
    using InputType = std::conditional_t<std::is_void_v<PrevR>, std::monostate, PrevR>;
    using StorageType = std::conditional_t<std::is_void_v<R>, std::monostate, R>;

    FTaskChainNode(F&& func, std::function<void()> onComplete) :Func(std::forward<F>(func)),OnComplete(std::move(onComplete)) {}
    void Post() {
        auto self= this->shared_from_this();
        if constexpr (!std::is_void_v<PrevR>) {
            InjectInputFromContext(Owner->DataContext);
        }
        auto [hTask, future] = GetTaskManagerSingleton()->AddTask(WorkflowHandle,
            [this, self]() {
                if constexpr (std::is_void_v<R>) {
                    if constexpr (std::is_void_v<PrevR>) {
                        Func();
                    }
                    else {
                        Func(std::move(*Input));
                    }
                }
                else {
                    if constexpr (std::is_void_v<PrevR>) {
                        Ret = Func();
                    }
                    else {
                        Ret = Func(std::move(*Input));
                    }
                }
                ExtractResultToContext(Owner->DataContext);
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
        auto pFuture = std::make_shared<FFutureWrapper<void>>();
        pFuture->Future = std::move(future);
        Owner->pFuture = pFuture;
        Owner->TaskHandle = hTask;
    }

    void ExtractResultToContext(std::any& context) override {
        if constexpr (!std::is_void_v<R>) {
            context = std::move(Ret);
        }
    }

    void InjectInputFromContext(std::any& context) override {
        if constexpr (!std::is_void_v<PrevR>) {
            if (context.has_value()) {
                Input = std::any_cast<PrevR>(std::move(context));
            }
        }
    }

    F Func;
    std::function<void()> OnComplete;
    StorageType Ret{};
    std::optional<InputType> Input;
};
