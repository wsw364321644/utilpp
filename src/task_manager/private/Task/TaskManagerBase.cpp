#include "Task/TaskManagerBase.h"





FTaskManagerBase::FTaskManagerBase() {
    TaskWorkflowDatas.try_emplace(NullHandle, std::make_shared<TaskWorkflow_t>());
    MainThread = WorkflowHandle_t();
    TaskWorkflowDatas.try_emplace(MainThread, std::make_shared<TaskWorkflow_t>());
}

FTaskManagerBase::~FTaskManagerBase()
{
}




WorkflowHandle_t FTaskManagerBase::NewWorkflow()
{
    auto out = WorkflowHandle_t();
    AppendingAddTaskWorkflowDatas.enqueue(std::make_tuple(out, std::make_shared<TaskWorkflow_t>()));
    return out;
}

WorkflowHandle_t FTaskManagerBase::GetMainThread()
{
    return MainThread;
}

void FTaskManagerBase::ReleaseWorkflow(WorkflowHandle_t handle)
{
    if (GetMainThread() == handle) {
        return;
    }
    AppendingDelTaskWorkflowDatas.enqueue(handle);
}

void FTaskManagerBase::Run()
{
    auto pMainThreadData = TaskWorkflowDatas[GetMainThread()];;
    while (!bRequestExit.load()) {
        Tick();
        auto dur = pMainThreadData->TimeRecorder.GetDeltaToNow<std::chrono::nanoseconds>();
        if (dur < pMainThreadData->RepeatTime) {
            std::this_thread::sleep_for(pMainThreadData->RepeatTime - dur);
        }
    }
    Tick();
}

void FTaskManagerBase::Tick()
{
    auto SyncAddTaskDataNum = SyncAddTaskDatas.try_dequeue_bulk(SyncAddTaskDataBuf, 100);
    auto SyncDelTaskDataNum = SyncDelTaskDatas.try_dequeue_bulk(SyncDelTaskDataBuf, 100);
    do {
        auto count= AppendingAddTaskWorkflowDatas.try_dequeue_bulk(AppendingAddTaskWorkflowDataBuf, 5);
        if (count == 0) {
            break;
        }
        for (int i = 0; i < count; i++) {
            auto& [handle, ptr] = AppendingAddTaskWorkflowDataBuf[i];
            TaskWorkflowDatas.try_emplace(handle, ptr);
        }
    } while (true);

    do {
        auto count = AppendingDelTaskWorkflowDatas.try_dequeue_bulk(AppendingDelWorkflowHandleBuf, 5);
        if (count == 0) {
            break;
        }
        for (int i = 0; i < count; i++) {
            auto& TaskWorkflowData=TaskWorkflowDatas[AppendingDelWorkflowHandleBuf[i]];
            TaskWorkflowDatas.erase(AppendingDelWorkflowHandleBuf[i]);
            for (auto& [taskHandle,pTaskData] : TaskWorkflowData->Tasks) {
                Tasks.erase(taskHandle);
            }
            for (auto& [taskHandle, pTaskData] : TaskWorkflowData->TimerTasks) {
                TimerTasks.erase(taskHandle);
            }
            for (auto& [taskHandle, pTaskData] : TaskWorkflowData->TickTasks) {
                TickTasks.erase(taskHandle);
            }
            for (auto& [taskHandle, pTaskData] : TaskWorkflowData->CancelableTasks) {
                CancelableTasks.erase(taskHandle);
            }
        }
    } while (true);

    for (int i = 0; i < SyncAddTaskDataNum; i++) {
        auto& [func] = SyncAddTaskDataBuf[i];
        func();
    }

    for (int i = 0; i < SyncDelTaskDataNum; i++) {
        auto& [TaskHandle] = SyncDelTaskDataBuf[i];
        auto TickTaskItr =TickTasks.find(TaskHandle);
        if (TickTaskItr != TickTasks.end()) {
            auto& [taskHandle, pTaskData] = *TickTaskItr;
            TaskWorkflowDatas[pTaskData->Workflow]->AppendingDelTaskDatas.enqueue(std::make_tuple(taskHandle, pTaskData));
            TickTasks.erase(TickTaskItr);
            continue;
        }
        auto TimerTaskItr = TimerTasks.find(TaskHandle);
        if (TimerTaskItr != TimerTasks.end()) {
            auto& [taskHandle, pTaskData] = *TimerTaskItr;
            TaskWorkflowDatas[pTaskData->Workflow]->AppendingDelTaskDatas.enqueue(std::make_tuple(taskHandle, pTaskData));
            TimerTasks.erase(TimerTaskItr);
            continue;
        }
        auto TaskItr = Tasks.find(TaskHandle);
        if (TaskItr != Tasks.end()) {
            auto& [taskHandle, pTaskData] = *TaskItr;
            TaskWorkflowDatas[pTaskData->Workflow]->AppendingDelTaskDatas.enqueue(std::make_tuple(taskHandle, pTaskData));
            Tasks.erase(TaskItr);
            continue;
        }
        auto CancelableTaskItr = CancelableTasks.find(TaskHandle);
        if (CancelableTaskItr != CancelableTasks.end()) {
            auto& [taskHandle, pTaskData] = *CancelableTaskItr;
            TaskWorkflowDatas[pTaskData->Workflow]->AppendingDelTaskDatas.enqueue(std::make_tuple(taskHandle, pTaskData));
            CancelableTasks.erase(CancelableTaskItr);
            continue;
        }
    }

    auto SyncFinishedTaskDataNum = SyncFinishedTaskDatas.try_dequeue_bulk(SyncFinishedTaskDataBuf, 100);
    for (int i = 0; i < SyncFinishedTaskDataNum; i++) {
        auto& [taskHandle,pTaskData] = SyncFinishedTaskDataBuf[i];
        switch (pTaskData->Type) {
        case ETaskType::TT_Common: {
            Tasks.erase(taskHandle);
            break;
        }
        case ETaskType::TT_Cancelable: {
            CancelableTasks.erase(taskHandle);
            break;
        }
        default: {
            assert(false && "impossible finished type");
        }
        }
    }
}

void FTaskManagerBase::Stop()
{
    bRequestExit = true;
}


CommonTaskHandle_t FTaskManagerBase::AddCancelableTask(WorkflowHandle_t handle, FCancelableTaskBase* task)
{
    auto TaskHandle = CommonTaskHandle_t();
    auto AddFunc =
        [&, handle, task, TaskHandle]() {
        auto itr = TaskWorkflowDatas.find(handle);
        if (itr == TaskWorkflowDatas.end()) {
            return;
        }
        auto [taskItr, res] = CancelableTasks.try_emplace(TaskHandle, std::make_shared<CancelableTaskData_t>(handle, task));
        if (!res) {
            return;
        }
        auto& [wCommonHandle, pWorkflowData] = *itr;
        auto& [tCommonHandle, pTaskData] = *taskItr;
        pWorkflowData->AppendingAddTaskDatas.enqueue(std::make_tuple(tCommonHandle, pTaskData));
        }
    ;
    SyncAddTaskDatas.enqueue(std::make_tuple(AddFunc));
    return TaskHandle;
}

CommonTaskHandle_t FTaskManagerBase::AddTick(WorkflowHandle_t handle, TTickTask task)
{
    auto TaskHandle = CommonTaskHandle_t();
    auto AddFunc =
        [&, handle, task, TaskHandle]() {
        auto itr = TaskWorkflowDatas.find(handle);
        if (itr == TaskWorkflowDatas.end()) {
            return;
        }
        auto [taskItr, res] = TickTasks.try_emplace(TaskHandle, std::make_shared<TickTaskData_t>(handle, task));
        if (!res) {
            return;
        }
        auto& [wCommonHandle, pWorkflowData] = *itr;
        auto& [tCommonHandle, pTaskData] = *taskItr;
        pWorkflowData->AppendingAddTaskDatas.enqueue(std::make_tuple(tCommonHandle, pTaskData));
        }
    ;
    SyncAddTaskDatas.enqueue(std::make_tuple(AddFunc));
    return TaskHandle;
}

CommonTaskHandle_t FTaskManagerBase::AddTimer(WorkflowHandle_t handle, TTimerTask task, uint64_t repeat, uint64_t timeout)
{
    auto TaskHandle = CommonTaskHandle_t();
    auto AddFunc =
        [&, handle, task, repeat, timeout, TaskHandle]() {
        auto itr = TaskWorkflowDatas.find(handle);
        if (itr == TaskWorkflowDatas.end()) {
            return;
        }
        auto [taskItr, res] = TimerTasks.try_emplace(TaskHandle, std::make_shared < TimerTaskData_t>(handle, task, std::chrono::nanoseconds(repeat), std::chrono::nanoseconds(timeout)));
        if (!res) {
            return;
        }
        auto& [wCommonHandle, pWorkflowData] = *itr;
        auto& [tCommonHandle, pTaskData] = *taskItr;
        pWorkflowData->AppendingAddTaskDatas.enqueue(std::make_tuple(tCommonHandle, pTaskData));
        }
    ;
    SyncAddTaskDatas.enqueue(std::make_tuple(AddFunc));
    return TaskHandle;
}

CommonTaskHandle_t FTaskManagerBase::AddTaskNoReturn(WorkflowHandle_t handle, TCommonTask task)
{
    auto TaskHandle = CommonTaskHandle_t();
    auto AddFunc = 
        [&, handle, task, TaskHandle]() {
        auto itr = TaskWorkflowDatas.find(handle);
        if (itr == TaskWorkflowDatas.end()) {
            return;
        }
        auto [taskItr,res] = Tasks.try_emplace(TaskHandle, std::make_shared < CommonTaskData_t>(handle, task));
        if (!res) {
            return;
        }
        auto& [wCommonHandle,pWorkflowData] = *itr;
        auto& [tCommonHandle, pTaskData] = *taskItr;
        pWorkflowData->AppendingAddTaskDatas.enqueue(std::make_tuple(tCommonHandle, pTaskData));
        }
    ;
    SyncAddTaskDatas.enqueue(std::make_tuple(AddFunc));
    return TaskHandle;
}

void FTaskManagerBase::RemoveTask(CommonTaskHandle_t handle)
{
    SyncDelTaskDatas.enqueue(handle);
}

void FTaskManagerBase::WorkflowThreadTick(std::shared_ptr<TaskWorkflow_t> pWorkflowData)
{
    do {
        auto count = pWorkflowData->AppendingAddTaskDatas.try_dequeue_bulk(pWorkflowData->AppendingTaskDataBuf, 5);
        if (count == 0) {
            break;
        }
        for (int i = 0; i < count; i++) {
            auto& [tHandle, pTaskData] = pWorkflowData->AppendingTaskDataBuf[i];
            switch (pTaskData->Type) {
            case ETaskType::TT_Common: {
                auto derivedPtr = std::dynamic_pointer_cast<CommonTaskData_t>(pTaskData);
                pWorkflowData->Tasks.try_emplace(tHandle, derivedPtr);
                break;
            }
            case ETaskType::TT_Timer: {
                auto derivedPtr = std::dynamic_pointer_cast<TimerTaskData_t>(pTaskData);
                pWorkflowData->TimerTasks.try_emplace(tHandle, derivedPtr);
                break;
            }
            case ETaskType::TT_Tick: {
                auto derivedPtr = std::dynamic_pointer_cast<TickTaskData_t>(pTaskData);
                pWorkflowData->TickTasks.try_emplace(tHandle, derivedPtr);
                break;
            }
            case ETaskType::TT_Cancelable: {
                auto derivedPtr = std::dynamic_pointer_cast<CancelableTaskData_t>(pTaskData);
                pWorkflowData->CancelableTasks.try_emplace(tHandle, derivedPtr);
                derivedPtr->Task->Init(tHandle);
                break;
            }
            }
        }
    } while (true);

}

void FTaskManagerBase::ThreadFinishTask(CommonHandle_t tHandle,std::shared_ptr<TaskDataBase_t> data)
{
    SyncFinishedTaskDatas.enqueue(std::make_tuple(tHandle, data));
}
