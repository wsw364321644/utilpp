#pragma once
#include "Task/TaskManagerBase.h"

#include <taskflow/taskflow.hpp>


class  FTaskFlowTaskManager :public FTaskManagerBase {
public:
    static FTaskFlowTaskManager* Get();
    void Tick() override;
private:
    FTaskFlowTaskManager();
    void TickTaskWorkflow(std::shared_ptr<TaskWorkflow_t> pWorkflowData);
    //void TickRandomTaskWorkflow();
    tf::Executor Executor;
};