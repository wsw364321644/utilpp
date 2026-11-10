#pragma once

#include "Task/TaskManager.h"
#include "task_manager_export_defs.h"

#include <any>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <queue>
#include <tuple>
#include <type_traits>
#include <utility>

// -----------------------------------------------------------------------------
// Task-chain infrastructure
// -----------------------------------------------------------------------------

struct TaskChain_t;

class ITaskChainNodeBase {
public:
    virtual ~ITaskChainNodeBase() = default;

    void SetWorkflow(WorkflowHandle_t workflowHandle) {
        WorkflowHandle = workflowHandle;
    }

    void SetOwner(const std::shared_ptr<TaskChain_t>& owner) {
        Owner = owner;
    }

    virtual void Post() = 0;

protected:
    WorkflowHandle_t WorkflowHandle{};
    std::weak_ptr<TaskChain_t> Owner;
};

class IFutureWrapperBase {
public:
    virtual ~IFutureWrapperBase() = default;
    virtual bool IsComplete() = 0;
};

template <class R>
class FFutureWrapper : public IFutureWrapperBase {
public:
    bool IsComplete() override {
        return Future.valid() &&
            Future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    std::future<R> Future;
};

struct TaskChain_t {
    std::queue<std::shared_ptr<ITaskChainNodeBase>> TaskChainNodes;
    std::shared_ptr<IFutureWrapperBase> pFuture;
    CommonTaskHandle_t TaskHandle{};
    std::any DataContext;
};

// -----------------------------------------------------------------------------
// Return-value normalization.
//
// Every node writes exactly the type expected by the next node into DataContext:
//
//   void                  -> std::tuple<>
//   T                     -> std::tuple<T>
//   std::pair<A, B>       -> std::tuple<A, B>
//   std::tuple<A, ...>    -> std::tuple<A, ...>
//
// The conversion below changes both the C++ type and the actual stored value.
// -----------------------------------------------------------------------------

template <typename T>
struct TIsStdTuple : std::false_type {};

template <typename... Ts>
struct TIsStdTuple<std::tuple<Ts...>> : std::true_type {};

template <typename T>
inline constexpr bool TIsStdTupleV = TIsStdTuple<std::remove_cv_t<T>>::value;

template <typename T>
struct TIsStdPair : std::false_type {};

template <typename A, typename B>
struct TIsStdPair<std::pair<A, B>> : std::true_type {};

template <typename T>
inline constexpr bool TIsStdPairV = TIsStdPair<std::remove_cv_t<T>>::value;

template <typename T>
struct TNormalizeTaskResult {
    using type = std::tuple<std::decay_t<T>>;
};

template <>
struct TNormalizeTaskResult<void> {
    using type = std::tuple<>;
};

template <typename... Ts>
struct TNormalizeTaskResult<std::tuple<Ts...>> {
    using type = std::tuple<std::decay_t<Ts>...>;
};

template <typename A, typename B>
struct TNormalizeTaskResult<std::pair<A, B>> {
    using type = std::tuple<std::decay_t<A>, std::decay_t<B>>;
};

template <typename T>
using TNormalizeTaskResultT =
typename TNormalizeTaskResult<std::remove_cv_t<T>>::type;

template <typename T>
auto MakeTaskResultTuple(T&& value) {
    using ValueType = std::decay_t<T>;

    if constexpr (TIsStdTupleV<ValueType>) {
        return ValueType(std::forward<T>(value));
    }
    else if constexpr (TIsStdPairV<ValueType>) {
        return std::make_tuple(
            std::forward<T>(value).first,
            std::forward<T>(value).second);
    }
    else {
        return std::make_tuple(std::forward<T>(value));
    }
}

// Computes the result of invoking F as an lvalue with the elements of Tuple
// expanded as independent arguments.
template <typename F, typename Tuple>
struct TTaskChainInvokeResult;

template <typename F, typename... Args>
struct TTaskChainInvokeResult<F, std::tuple<Args...>> {
    using type = std::invoke_result_t<F&, Args...>;
};

template <typename F, typename Tuple>
using TTaskChainInvokeResultT =
typename TTaskChainInvokeResult<F, Tuple>::type;

// -----------------------------------------------------------------------------
// A node consumes one tuple and produces one normalized result tuple.
// -----------------------------------------------------------------------------

template <typename PrevTuple, typename F>
class FTaskChainNode
    : public ITaskChainNodeBase,
    public std::enable_shared_from_this<FTaskChainNode<PrevTuple, F>> {
    static_assert(TIsStdTupleV<PrevTuple>,
        "PrevTuple must be a std::tuple type.");

public:
    using RawResult = TTaskChainInvokeResultT<F, PrevTuple>;
    using ResultTuple = TNormalizeTaskResultT<RawResult>;

    template <typename Fn>
    FTaskChainNode(Fn&& func, std::function<void()> onComplete)
        : Func(std::forward<Fn>(func)),
        OnComplete(std::move(onComplete)) {}

    void Post() override {
        auto self = this->shared_from_this();
        auto owner = Owner.lock();
        if (!owner) {
            return;
        }
        if constexpr (std::tuple_size_v<PrevTuple> != 0) {
            // The previous node must have stored exactly PrevTuple.
            if (owner->DataContext.type() != typeid(PrevTuple)) {
                throw std::runtime_error("TaskChain input type mismatch");
            }
        }

        auto [hTask, future] = GetTaskManagerSingleton()->AddTask(
            WorkflowHandle,
            [this, self, owner]() mutable {
                if constexpr (std::is_void_v<RawResult>) {
                    std::apply(
                        [this](auto&&... args) {
                            std::invoke(Func,
                                std::forward<decltype(args)>(args)...);
                        },
                        GetInputTuple(owner));

                    owner->DataContext = std::tuple<>{};
                }
                else {
                    owner->DataContext = std::apply(
                        [this](auto&&... args) {
                            return MakeTaskResultTuple(
                                std::invoke(Func,
                                    std::forward<decltype(args)>(args)...));
                        },
                        GetInputTuple(owner));
                }

                if (WorkflowHandle == GetTaskManagerSingleton()->GetMainThread()) {
                    OnComplete();
                }
                else {
                    GetTaskManagerSingleton()->AddTask(
                        GetTaskManagerSingleton()->GetMainThread(),
                        [this, self]() {
                            OnComplete();
                        }
                    );
                }
            }
        );

        auto pFuture = std::make_shared<FFutureWrapper<void>>();
        pFuture->Future = std::move(future);
        owner->pFuture = std::move(pFuture);
        owner->TaskHandle = hTask;
    }


private:
    // The first node has PrevTuple == tuple<> and needs no DataContext input.
    // For all other nodes, Input is populated before the task is posted.
    decltype(auto) GetInputTuple(std::shared_ptr<TaskChain_t>& owner) {
        if constexpr (std::tuple_size_v<PrevTuple> == 0) {
            return std::tuple<>{};
        }
        else {
            return std::move(
                std::any_cast<PrevTuple&>(owner->DataContext));
        }
    }

    F Func;
    std::function<void()> OnComplete;
};
