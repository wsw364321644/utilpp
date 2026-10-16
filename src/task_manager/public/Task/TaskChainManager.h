#pragma once
#include "Task/TaskManager.h"
#include "Task/TaskChain.h"
#include "task_manager_export_defs.h"

#include <singleton.h>
#include <future>
#include <queue>
#include <unordered_map>
#include <assert.h>

class FTaskChainManager:public TProvideSingletonClass<FTaskChainManager> {
public:
    //not thread safe
    CommonHandle32_t NewTaskChain() {
        auto emplaceRes = TaskChains.try_emplace(CommonHandle32_t::atomic_count);
        emplaceRes.first->second = std::make_shared<TaskChain_t>();
        return emplaceRes.first->first;
    }

    template <typename PrevR,typename F>
    void ChainTask(CommonHandle32_t handle, WorkflowHandle_t hWorkflow, F&& func) {
        auto itr=TaskChains.find(handle);
        if (itr == TaskChains.end()) {
            return;
        }
        auto& chain = itr->second;
        auto& pTaskChainNode =chain->TaskChainNodes.emplace();
        pTaskChainNode = std::make_shared<FTaskChainNode<PrevR,std::decay_t<F>>>(std::forward<F>(func),
            [this, handle]() {
                PostChainNode(handle);
            }
        );
        pTaskChainNode->SetOwner(chain.get());
        pTaskChainNode->SetWorkflow(hWorkflow);
    }

    template <typename F>
    void ChainTask(CommonHandle32_t handle, WorkflowHandle_t hWorkflow, F&& func) {
        ChainTask<void>(handle, hWorkflow, std::forward<F>(func));
    }

    bool SealChain(CommonHandle32_t handle) {
        auto itr = TaskChains.find(handle);
        if (itr == TaskChains.end()) {
            return false;
        }
        if (itr->second->TaskChainNodes.size() == 0) {
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
        if (itr->second->TaskChainNodes.size() == 0) {
            TaskChains.erase(itr);
        }
        else {
            auto& pNode = itr->second->TaskChainNodes.front();
            pNode->Post();
            itr->second->TaskChainNodes.pop();
        }
    }

    void Cancel(CommonHandle32_t handle) {
        TaskChains.erase(handle);
    }
    std::unordered_map<CommonHandle32_t, std::shared_ptr<TaskChain_t>> TaskChains;
};

TASK_MANAGER_EXPORT FTaskChainManager* GetTaskChainManagerSingleton();



template <typename PrevR=void>
class FTaskChainBuilder {
public:
    FTaskChainBuilder(CommonHandle32_t handle,WorkflowHandle_t workflow)
        :m_chain(handle),  m_workflow(workflow) {}
    FTaskChainBuilder(CommonHandle32_t handle) :FTaskChainBuilder(handle, NullHandle) {}
    FTaskChainBuilder(WorkflowHandle_t workflow) :FTaskChainBuilder(NullHandle, workflow) {}
    FTaskChainBuilder() :FTaskChainBuilder(NullHandle, NullHandle) {}


    template <typename F>
    auto Then(F&& func, WorkflowHandle_t workflow) {

        using FuncType = std::decay_t<F>;
        using R = SafeInvokeResult_t<std::decay_t<F>, PrevR>;

        if (!m_chain) {
            m_chain=GetTaskChainManagerSingleton()->NewTaskChain();
        }
        GetTaskChainManagerSingleton()->ChainTask<PrevR>(m_chain, workflow, std::forward<F>(func));

        return FTaskChainBuilder<R>(m_chain, m_workflow);
    }

    template <typename F>
    auto Then(F&& func) {
        assert(m_workflow.IsValid());
        return Then(std::forward<F>(func), m_workflow);
    }

    bool Steal() {
        return GetTaskChainManagerSingleton()->SealChain(m_chain);
    }

    CommonHandle32_t GetHandle() {
        return m_chain;
    }
private:
    WorkflowHandle_t m_workflow;
    CommonHandle32_t m_chain;
};
