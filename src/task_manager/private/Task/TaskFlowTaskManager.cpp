#include "Task/TaskFlowTaskManager.h"

FTaskFlowTaskManager::FTaskFlowTaskManager() {

}
FTaskFlowTaskManager* FTaskFlowTaskManager::Get()
{
    static std::atomic<std::shared_ptr<FTaskFlowTaskManager>> AtomicTaskManager;
    auto oldptr = AtomicTaskManager.load();
    if (!oldptr) {
        std::shared_ptr<FTaskFlowTaskManager> pTaskManager(new FTaskFlowTaskManager);
        AtomicTaskManager.compare_exchange_strong(oldptr, pTaskManager);
    }
    return AtomicTaskManager.load().get();
}

void FTaskFlowTaskManager::Tick() {
    FTaskManagerBase::Tick();
    for (auto& [wHandle, pTaskWorkflow] : TaskWorkflowDatas) {
        if (wHandle == MainThread)
        {
            TickTaskWorkflow(pTaskWorkflow);
        }
        else {
            auto& optfuture = pTaskWorkflow->OptFuture;
            if (optfuture.has_value() && optfuture.value().wait_for(std::chrono::seconds(0)) == std::future_status::timeout) {
                continue;
            }
            optfuture = Executor.async("", [&,pTaskWorkflow]() {
                TickTaskWorkflow(pTaskWorkflow);
                });
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

    for (auto& [tHandle, pTaskData] : pWorkflowData->TimerTasks) {
        if (pTaskData->Timeout <= deltime) {
            pTaskData->Timeout = pTaskData->Repeat;
            pTaskData->Task(tHandle);
        }
        else {
            pTaskData->Timeout -= deltime;
        }
    }

    if (needTick) {
        for (auto& [tHandle, pTaskData] : pWorkflowData->CancelableTasks) {
            pTaskData->Task->Tick(delsec);
        }
    }

    for (auto& [tHandle, pTaskData] : pWorkflowData->Tasks) {
        pTaskData->Task();
        SyncFinishedTaskDatas.enqueue(std::make_tuple(tHandle, pTaskData));
    }
    pWorkflowData->Tasks.clear();
}