#pragma once

#include "Task/TaskManager.h"
#include "Task/TaskChain.h"
#include "task_manager_export_defs.h"

#include <singleton.h>
#include <cassert>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

class FTaskChainManager : public TProvideSingletonClass<FTaskChainManager> {
public:
    // Not thread-safe. Call chain-building and cancellation APIs from one thread.
    CommonHandle32_t NewTaskChain() {
        auto emplaceRes = TaskChains.try_emplace(CommonHandle32_t::atomic_count);
        emplaceRes.first->second = std::make_shared<TaskChain_t>();
        return emplaceRes.first->first;
    }

    template <typename PrevTuple, typename F>
    void ChainTask(CommonHandle32_t handle, WorkflowHandle_t hWorkflow, F&& func) {
        auto itr = TaskChains.find(handle);
        if (itr == TaskChains.end()) {
            return;
        }

        auto& chain = itr->second;
        auto& pTaskChainNode = chain->TaskChainNodes.emplace();
        pTaskChainNode =
            std::make_shared<FTaskChainNode<PrevTuple, std::decay_t<F>>>(
                std::forward<F>(func),
                [this, handle]() {
                    PostChainNode(handle);
                }
            );

        pTaskChainNode->SetOwner(chain);
        pTaskChainNode->SetWorkflow(hWorkflow);
    }

    // First node: no previous result, so its input type is tuple<>.
    template <typename F>
    void ChainTask(CommonHandle32_t handle, WorkflowHandle_t hWorkflow, F&& func) {
        ChainTask<std::tuple<>>(handle, hWorkflow, std::forward<F>(func));
    }

    bool SealChain(CommonHandle32_t handle) {
        auto itr = TaskChains.find(handle);
        if (itr == TaskChains.end() || itr->second->TaskChainNodes.empty()) {
            return false;
        }

        PostChainNode(handle);
        return true;
    }

    void PostChainNode(CommonHandle32_t handle) {
        auto itr = TaskChains.find(handle);
        if (itr == TaskChains.end()) {
            return;
        }

        auto& chain = itr->second;
        if (chain->TaskChainNodes.empty()) {
            TaskChains.erase(itr);
            return;
        }

        auto pNode = chain->TaskChainNodes.front();
        chain->TaskChainNodes.pop();
        pNode->Post();
    }

    void Cancel(CommonHandle32_t handle) {
        TaskChains.erase(handle);
    }

    std::unordered_map<CommonHandle32_t, std::shared_ptr<TaskChain_t>> TaskChains;
};

TASK_MANAGER_EXPORT FTaskChainManager* GetTaskChainManagerSingleton();

// -----------------------------------------------------------------------------
// Convert a normalized tuple type into the corresponding builder type.
// -----------------------------------------------------------------------------
template <typename... PrevRs>
class FTaskChainBuilder;

template <typename Tuple>
struct TMakeTaskChainBuilder;

template <typename... Ts>
struct TMakeTaskChainBuilder<std::tuple<Ts...>> {
    using type = FTaskChainBuilder<Ts...>;
};

template <typename Tuple>
using TMakeTaskChainBuilderT = typename TMakeTaskChainBuilder<Tuple>::type;

// -----------------------------------------------------------------------------
// Builder. PrevRs... are the arguments accepted by the next Then() callback.
// Internally, values are always transported as std::tuple<PrevRs...>.
// -----------------------------------------------------------------------------

template <typename... PrevRs>
class FTaskChainBuilder {
    using PrevTuple = std::tuple<PrevRs...>;

public:
    FTaskChainBuilder(CommonHandle32_t handle, WorkflowHandle_t workflow)
        : m_workflow(workflow), m_chain(handle) {}

    explicit FTaskChainBuilder(CommonHandle32_t handle)
        : FTaskChainBuilder(handle, NullHandle) {}

    explicit FTaskChainBuilder(WorkflowHandle_t workflow)
        : FTaskChainBuilder(NullHandle, workflow) {}

    FTaskChainBuilder()
        : FTaskChainBuilder(NullHandle, NullHandle) {}

    FTaskChainBuilder(const FTaskChainBuilder&) = delete;
    FTaskChainBuilder& operator=(const FTaskChainBuilder&) = delete;
    FTaskChainBuilder(FTaskChainBuilder&&) noexcept = default;
    FTaskChainBuilder& operator=(FTaskChainBuilder&&) noexcept = default;
    ~FTaskChainBuilder() {
    }

    template <typename F>
    auto Then(F&& func, WorkflowHandle_t workflow) {
        using FuncType = std::decay_t<F>;
        using RawResult = TTaskChainInvokeResultT<FuncType, PrevTuple>;
        using NextTuple = TNormalizeTaskResultT<RawResult>;
        using NextBuilder = TMakeTaskChainBuilderT<NextTuple>;

        if (!m_chain) {
            m_chain = GetTaskChainManagerSingleton()->NewTaskChain();
        }

        GetTaskChainManagerSingleton()->ChainTask<PrevTuple>(
            m_chain, workflow, std::forward<F>(func));

        auto outbuilder= NextBuilder{ m_chain, m_workflow };
        m_chain = NullHandle;
        return outbuilder;
    }

    template <typename F>
    auto Then(F&& func) {
        assert(m_workflow.IsValid());
        return Then(std::forward<F>(func), m_workflow);
    }

    bool Seal() {
        auto bres= GetTaskChainManagerSingleton()->SealChain(m_chain);
        m_chain = NullHandle;
        return bres;
    }

    CommonHandle32_t GetHandle() const {
        return m_chain;
    }

private:
    WorkflowHandle_t m_workflow;
    CommonHandle32_t m_chain;
};
