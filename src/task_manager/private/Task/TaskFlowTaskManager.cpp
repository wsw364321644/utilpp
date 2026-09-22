#include "Task/TaskFlowTaskManager.h"
#include <singleton.h>
FTaskFlowTaskManager::FTaskFlowTaskManager() {

}
FTaskFlowTaskManager* FTaskFlowTaskManager::Get()
{
    return TClassSingletonHelper<FTaskFlowTaskManager>::GetClassSingleton().get();
}

void FTaskFlowTaskManager::Tick() {
    FTaskManagerBase::Tick();
    for (auto& [wHandle, pTaskWorkflow] : TaskWorkflowDatas) {
        if (wHandle == MainThread)
        {
            TickTaskWorkflow(pTaskWorkflow);
        }
        else {
            if (!bRequestExit) {
                auto& optfuture = pTaskWorkflow->OptFuture;
                if (optfuture.has_value() && optfuture.value().wait_for(std::chrono::seconds(0)) == std::future_status::timeout) {
                    continue;
                }
                optfuture = Executor.async(
                    [&, pTaskWorkflow]() {
                        TickTaskWorkflow(pTaskWorkflow);
                    }
                );
            }
        }
    }
}

void FTaskFlowTaskManager::TickTaskWorkflow(std::shared_ptr<TaskWorkflow_t> pWorkflowData)
{
    WorkflowThreadTick(pWorkflowData);
    thread_local tf::Taskflow Taskflow;
    Taskflow.clear();

    pWorkflowData->TimeRecorder.Tick();
    auto deltime = pWorkflowData->TimeRecorder.GetDelta<std::chrono::nanoseconds>();
    auto delsec = float(deltime.count()) / std::chrono::nanoseconds::period::den;
    bool needTick;
    if (pWorkflowData->Timeout <= deltime) {
        pWorkflowData->Timeout = pWorkflowData->RepeatTime;
        needTick = true;
    }
    else {
        pWorkflowData->Timeout -= deltime;
        needTick = false;
    }

    if (needTick) {
        for (auto& [tHandle, pTaskData] : pWorkflowData->TickTasks) {
            pTaskData->Task(delsec);
        }
    }

    for (auto itr = pWorkflowData->TimerTasks.begin(); itr != pWorkflowData->TimerTasks.end();) {
        auto& [tHandle, pTaskData] = *itr;
        if (pTaskData->Timeout <= deltime) {
            pTaskData->Task(tHandle);
            if (pTaskData->Repeat == std::chrono::nanoseconds(0)) {
                ThreadFinishTask(tHandle, pTaskData);
                itr=pWorkflowData->TimerTasks.erase(itr);
                continue;
            }
            else {
                pTaskData->Timeout = pTaskData->Repeat;
            }
        }
        else {
            pTaskData->Timeout -= deltime;
        }
        itr++;
    }

    if (needTick) {
        for (auto& [tHandle, pTaskData] : pWorkflowData->CancelableTasks) {
            pTaskData->Task->Tick(delsec);
        }
    }

    for (auto& [tHandle, pTaskData] : pWorkflowData->Tasks) {
        pTaskData->Task();
        ThreadFinishTask(tHandle, pTaskData);
    }
    pWorkflowData->Tasks.clear();
}