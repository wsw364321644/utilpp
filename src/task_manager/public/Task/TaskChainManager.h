#pragma once
#include "Task/TaskManager.h"
#include "Task/TaskChain.h"
#include "task_manager_export_defs.h"

#include <singleton.h>
#include <future>
#include <queue>
#include <unordered_map>

class FTaskChainManager:public TProvideSingletonClass<FTaskChainManager> {
public:
    //not thread safe
    CommonHandle32_t NewTaskChain() {
        auto emplaceRes = TaskChains.try_emplace(CommonHandle32_t::atomic_count);
        emplaceRes.first->second = std::make_shared<TaskChain_t>();
        return emplaceRes.first->first;
    }
    template <typename F, typename R = std::invoke_result_t<std::decay_t<F>>, std::enable_if_t<!std::is_invocable_v<std::decay_t<F>, std::shared_ptr<std::promise<R>>>, int> = 0>
    void ChainTask(CommonHandle32_t handle, WorkflowHandle_t hWorkflow, F&& func) {
        auto itr=TaskChains.find(handle);
        if (itr == TaskChains.end()) {
            return;
        }
        auto& chain = itr->second;
        auto& pTaskChainNode =chain->TaskChainNodes.emplace();
        pTaskChainNode = std::make_shared<FTaskChainNode<std::decay_t<F>>>(std::forward<F>(func),
            [this, handle]() {
                PostChainNode(handle);
            }
        );
        pTaskChainNode->SetOwner(chain.get());
        pTaskChainNode->SetWorkflow(hWorkflow);
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