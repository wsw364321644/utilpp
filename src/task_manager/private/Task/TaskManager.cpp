#include "Task/TaskManager.h"
#include <taskflow/taskflow.hpp>

std::atomic_uint32_t WorkflowHandle_t::WorkflowCount;
std::atomic_uint32_t CommonTaskHandle_t::TaskCount;

typedef struct TaskManagerInterData_t {
    tf::Executor Executor;
}TaskManagerInterData_t;

TaskManagerInterData_t* GetTaskManagerInterDataPtr(void* ptr) {
    return static_cast<TaskManagerInterData_t*>(ptr);
}
//WorkflowInterData_t* GetWorkflowInterDataPtr(void* ptr) {
//    return static_cast<WorkflowInterData_t*>(ptr);
//}

FTaskManager::FTaskManager() {
    InterData = new TaskManagerInterData_t;

    TaskWorkflowDatas.try_emplace(CommonHandle_t(NullHandle), std::make_shared<TaskWorkflow_t>());
    MainThread = NewWorkflow();
}

FTaskManager::~FTaskManager()
{
    if (InterData) {
        delete InterData;
    }
}

void FTaskManager::TickTaskWorkflow(WorkflowHandle_t handle)
{
    std::shared_lock TaskWorkflowLock{ TaskWorkflowMutex, std::defer_lock };
    std::unique_lock TaskLock{ TaskMutex, std::defer_lock };
    std::shared_lock TaskRLock{ TaskMutex, std::defer_lock };
    std::shared_lock TickRLock{ TickMutex, std::defer_lock };
    std::shared_lock TimerRLock{ TimerMutex, std::defer_lock };
    std::shared_ptr<TaskWorkflow_t> pTaskWorkflow;
    {
        std::scoped_lock lock(TaskWorkflowLock);
        if (TaskWorkflowDatas.find(handle) == TaskWorkflowDatas.end()) {
            return;
        }
        pTaskWorkflow = TaskWorkflowDatas[handle];
    }
    pTaskWorkflow->TimeRecorder.Tick();
    auto deltime = pTaskWorkflow->TimeRecorder.GetDelta<std::chrono::nanoseconds>();
    auto delsec = float(deltime.count()) / std::chrono::nanoseconds::period::den;
    bool needTick;
    if (pTaskWorkflow->Timeout <= deltime) {
        pTaskWorkflow->Timeout = pTaskWorkflow->RepeatTime;
        needTick = true;
    }
    else {
        pTaskWorkflow->Timeout -= deltime;
        needTick = false;
    } 

    if (needTick) {
        std::unordered_map < CommonHandle_t, std::shared_ptr<TickTaskData_t>> localTickTasks;
        std::vector<CommonTaskHandle_t> localThreadTickTasks;
        {
            std::scoped_lock lock(TickRLock);
            localTickTasks = TickTasks;
            localThreadTickTasks = pTaskWorkflow->TickTasks;
        }
        for (auto& task : localThreadTickTasks) {
            localTickTasks[task]->Task(delsec);
        }
    }
    
    {
        std::unordered_map < CommonHandle_t, std::shared_ptr<TimerTaskData_t>> localTimerTasks;
        std::vector<CommonTaskHandle_t> localThreadTasks;
        {
            std::scoped_lock lock(TimerRLock);
            localTimerTasks = TimerTasks;
            localThreadTasks = pTaskWorkflow->TimerTasks;
        }
        for (auto& handle : localThreadTasks) {
            if (localTimerTasks[handle]->Timeout <= deltime) {
                localTimerTasks[handle]->Timeout = localTimerTasks[handle]->Repeat;
                localTimerTasks[handle]->Task(handle);
            }
            else {
                localTimerTasks[handle]->Timeout -= deltime;
            }
        }
    }

    {
        std::set<CommonTaskHandle_t> localTasks;
        std::unordered_map < CommonHandle_t, std::shared_ptr<CommonTaskData_t>> localTaskDatas;
        {
            std::scoped_lock lock(TaskRLock);
            localTasks = pTaskWorkflow->Tasks;
            localTaskDatas = Tasks;
        }
        for (auto& task : localTasks) {
            std::shared_ptr<CommonTaskData_t> pTaskData;
            pTaskData = localTaskDatas[task];
            pTaskData->Task();
        }
        {
            std::scoped_lock lock(TaskLock);
            for (auto& task : localTasks) {
                Tasks.erase(task);
                pTaskWorkflow->Tasks.erase(task);
            }
        }
    }
}

void FTaskManager::TickRandonTaskWorkflow()
{
    std::shared_lock TaskWorkflowLock{ TaskWorkflowMutex, std::defer_lock };
    std::unique_lock TaskLock{ TaskMutex, std::defer_lock };
    std::shared_lock TaskRLock{ TaskMutex, std::defer_lock };
    std::shared_lock TickRLock{ TickMutex, std::defer_lock };
    std::shared_lock TimerRLock{ TimerMutex, std::defer_lock };
    auto& Executor = GetTaskManagerInterDataPtr(InterData)->Executor;
    tf::Taskflow Taskflow;
    std::shared_ptr<TaskWorkflow_t> pTaskWorkflow;
    {
        std::scoped_lock lock(TaskWorkflowLock);
        pTaskWorkflow = TaskWorkflowDatas[CommonHandle_t(NullHandle)];
    }
    pTaskWorkflow->TimeRecorder.Tick();
    auto deltime = pTaskWorkflow->TimeRecorder.GetDelta<std::chrono::nanoseconds>();
    auto delsec = float(deltime.count()) / std::chrono::nanoseconds::period::den;
    bool needTick;
    if (pTaskWorkflow->Timeout <= deltime) {
        pTaskWorkflow->Timeout = pTaskWorkflow->RepeatTime;
        needTick = true;
    }
    else {
        pTaskWorkflow->Timeout -= deltime;
        needTick = false;
    }

    if (needTick) {
        std::unordered_map < CommonHandle_t, std::shared_ptr<TickTaskData_t>> localTickTasks;
        std::vector<CommonTaskHandle_t> localThreadTickTasks;
        {
            std::scoped_lock lock(TickRLock);
            localTickTasks = TickTasks;
            localThreadTickTasks = pTaskWorkflow->TickTasks;
        }
        for (auto& task : localThreadTickTasks) {
            Taskflow.emplace(
                [task = localTickTasks[task]->Task, delsec]() {
                    task(delsec);
                }
            );
        }
    }

    {
        std::unordered_map < CommonHandle_t, std::shared_ptr<TimerTaskData_t>> localTimerTasks;
        std::vector<CommonTaskHandle_t> localThreadTasks;
        {
            std::scoped_lock lock(TimerRLock);
            localTimerTasks = TimerTasks;
            localThreadTasks = pTaskWorkflow->TimerTasks;
        }
        for (auto& handle : localThreadTasks) {
            if (localTimerTasks[handle]->Timeout <= deltime) {
                localTimerTasks[handle]->Timeout = localTimerTasks[handle]->Repeat;
                Taskflow.emplace(
                    [task = localTimerTasks[handle]->Task, handle]() {
                        task(handle);
                    }
                );
            }
            else {
                localTimerTasks[handle]->Timeout -= deltime;
            }
        }
    }

    std::set<CommonTaskHandle_t> localTasks;
    {
        std::unordered_map < CommonHandle_t, std::shared_ptr<CommonTaskData_t>> localTaskDatas;
        {
            std::scoped_lock lock(TaskRLock);
            localTasks = pTaskWorkflow->Tasks;
            localTaskDatas = Tasks;
        }
        for (auto& task : localTasks) {
            std::shared_ptr<CommonTaskData_t> pTaskData;
            pTaskData = localTaskDatas[task];
            Taskflow.emplace(
                [task = pTaskData->Task]() {
                    task();
                }
            );
        }
    }
    Executor.run(Taskflow).wait();
    {
        {
            std::scoped_lock lock(TaskLock);
            for (auto& task : localTasks) {
                Tasks.erase(task);
                pTaskWorkflow->Tasks.erase(task);
            }
        }
    }
}

FTaskManager* FTaskManager::Get()
{
    static std::atomic<std::shared_ptr<FTaskManager>> AtomicTaskManager;
    auto oldptr = AtomicTaskManager.load();
    if (!oldptr) {
        std::shared_ptr<FTaskManager> pTaskManager(new FTaskManager);
        AtomicTaskManager.compare_exchange_strong(oldptr, pTaskManager);
    }
    return AtomicTaskManager.load().get();
}

WorkflowHandle_t FTaskManager::NewWorkflow()
{
    std::scoped_lock lock(TaskWorkflowMutex);
    auto pair = TaskWorkflowDatas.try_emplace(WorkflowHandle_t::WorkflowCount, std::make_shared<TaskWorkflow_t>());
    if (!pair.second) {
        WorkflowHandle_t();
    }
    return pair.first->first;
}

WorkflowHandle_t FTaskManager::GetMainThread()
{
    return MainThread;
}

void FTaskManager::ReleaseWorkflow(WorkflowHandle_t handle)
{
    if (GetMainThread() == handle) {
        return;
    }
    std::scoped_lock lock(TaskWorkflowMutex);
    TaskWorkflowDatas.erase(handle);
}

void FTaskManager::Run()
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

void FTaskManager::Tick()
{
    std::shared_lock TaskWorkflowLock{ TaskWorkflowMutex, std::defer_lock };
    std::unique_lock TaskLock{ TaskMutex, std::defer_lock };
    std::shared_lock TaskRLock{ TaskMutex, std::defer_lock };
    TaskWorkflowLock.lock();
    for (auto& pair : TaskWorkflowDatas) {
        auto& pTaskWorkflow = pair.second;
        auto handle = pair.first;
        if (pair.first == MainThread)
        {
            TickTaskWorkflow(pair.first);
        }
        else if (pair.first == NullHandle) {
            auto& optfuture = pTaskWorkflow->OptFuture;
            if (optfuture.has_value() && optfuture.value().wait_for(std::chrono::seconds(0)) == std::future_status::timeout) {
                continue;
            }
            optfuture = GetTaskManagerInterDataPtr(InterData)->Executor.async("", [handle, this]() {
                TickRandonTaskWorkflow();
                });
        }
        else {
            auto& optfuture = pTaskWorkflow->OptFuture;
            if (optfuture.has_value() && optfuture.value().wait_for(std::chrono::seconds(0)) == std::future_status::timeout) {
                continue;
            }
            optfuture =GetTaskManagerInterDataPtr(InterData)->Executor.async("", [handle,this]() {
                TickTaskWorkflow(handle);
                });
            //tf::Taskflow taskflow;
            //tf::Task lastTFTask, curTFTask;

            //pTaskWorkflow->TimeRecorder.Tick();
            //auto deltime = pTaskWorkflow->TimeRecorder.GetDelta<std::chrono::nanoseconds>();
            //auto delsec = float(deltime.count()) / std::chrono::nanoseconds::period::den;
            //bool needTick;
            //if (pTaskWorkflow->Timeout <= deltime) {
            //    pTaskWorkflow->Timeout = pTaskWorkflow->RepeatTime;
            //    needTick = true;
            //}
            //else {
            //    pTaskWorkflow->Timeout -= deltime;
            //    needTick = false;
            //}
            //{
            //    if (needTick) {
            //        for (auto& task : pTaskWorkflow->TickTasks) {
            //            lastTFTask = curTFTask;
            //            auto newtick = std::bind(TickTasks[task]->Task, delsec);
            //            curTFTask = taskflow.emplace([newtick]() {newtick(); });
            //            if (!lastTFTask.empty()) {
            //                lastTFTask.precede(curTFTask);
            //            }
            //        }
            //    }
            //}
            //{
            //    for (auto& task : pTaskWorkflow->TimerTasks) {
            //        if (TimerTasks[task]->Timeout <= deltime) {
            //            TimerTasks[task]->Timeout = TimerTasks[task]->Repeat;
            //            lastTFTask = curTFTask;
            //            curTFTask = taskflow.emplace(TimerTasks[task]->Task);
            //            if (!lastTFTask.empty()) {
            //                lastTFTask.precede(curTFTask);
            //            }
            //        }
            //        else {
            //            TimerTasks[task]->Timeout -= deltime;
            //        }

            //    }
            //}
            //{
            //    std::scoped_lock lock(TaskRLock);
            //    for (auto& task : pTaskWorkflow->InProgressTasks) {
            //        auto itr = Tasks.find(task);
            //        if (itr != Tasks.end()) {
            //            itr->second->Delegate();
            //            Tasks.erase(itr);
            //        }
            //    }
            //}
            //{
            //    std::scoped_lock lock(TaskMutex);
            //    for (auto& task : pTaskWorkflow->Tasks) {
            //        lastTFTask = curTFTask;
            //        curTFTask = taskflow.emplace(Tasks[task]->Task);
            //        if (!lastTFTask.empty()) {
            //            lastTFTask.precede(curTFTask);
            //        }
            //    }
            //    pTaskWorkflow->InProgressTasks = pTaskWorkflow->Tasks;
            //    pTaskWorkflow->Tasks.clear();
            //}
            //optfuture = GetTaskManagerInterDataPtr(InterData)->Executor.run(taskflow);
            //optfuture.value().wait();
        }
    }


}

void FTaskManager::Stop()
{
    bRequestExit = true;
}


CommonTaskHandle_t FTaskManager::AddTick(WorkflowHandle_t handle, FTickTask task)
{
    std::scoped_lock lock(TaskWorkflowMutex, TickMutex);
    auto itr = TaskWorkflowDatas.find(handle);
    if (itr == TaskWorkflowDatas.end()) {
        return CommonTaskHandle_t();
    }
    auto res = TickTasks.emplace(CommonTaskHandle_t::TaskCount, std::make_shared<TickTaskData_t>(handle, task));
    if (!res.second) {
        return CommonTaskHandle_t();
    }
    itr->second->TickTasks.push_back(res.first->first);
    return res.first->first;
}

CommonTaskHandle_t FTaskManager::AddTimer(WorkflowHandle_t handle, FTimerTask task, uint64_t repeat, uint64_t timeout)
{
    std::scoped_lock lock(TaskWorkflowMutex, TimerMutex);
    auto itr = TaskWorkflowDatas.find(handle);
    if (itr == TaskWorkflowDatas.end()) {
        return CommonTaskHandle_t();
    }
    auto res = TimerTasks.emplace(CommonTaskHandle_t::TaskCount, std::make_shared < TimerTaskData_t>(handle, task, std::chrono::nanoseconds(repeat), std::chrono::nanoseconds(timeout)));
    if (!res.second) {
        return CommonTaskHandle_t();
    }
    itr->second->TimerTasks.push_back(res.first->first);
    return res.first->first;
}



void FTaskManager::RemoveTask(CommonTaskHandle_t handle)
{
    std::shared_lock TaskWorkflowLock{ TaskWorkflowMutex, std::defer_lock };
    std::unique_lock TaskLock{ TaskMutex, std::defer_lock };
    std::unique_lock TickLock{ TickMutex, std::defer_lock };
    std::unique_lock TimerLock{ TimerMutex, std::defer_lock };
    {
        std::scoped_lock lock(TaskWorkflowLock, TickMutex);
        auto itr = TickTasks.find(handle);
        if (itr != TickTasks.end()) {
            auto& pworkflow = TaskWorkflowDatas[itr->second->Workflow];
            auto tmpitr = std::find(pworkflow->TickTasks.begin(), pworkflow->TickTasks.end(), handle);
            if (tmpitr != pworkflow->TickTasks.end()) {
                pworkflow->TickTasks.erase(tmpitr);
            }
            TickTasks.erase(itr);
            return;
        }
    }
    {
        std::scoped_lock lock(TaskWorkflowLock, TimerMutex);
        auto itr = TimerTasks.find(handle);
        if (itr != TimerTasks.end()) {
            auto& pworkflow = TaskWorkflowDatas[itr->second->Workflow];
            auto tmpitr = std::find(pworkflow->TimerTasks.begin(), pworkflow->TimerTasks.end(), handle);
            if (tmpitr != pworkflow->TimerTasks.end()) {
                pworkflow->TimerTasks.erase(tmpitr);
            }
            TimerTasks.erase(itr);
        }
        return;
    }
    {
        std::scoped_lock lock(TaskWorkflowLock, TaskLock);
        auto itr = Tasks.find(handle);
        if (itr != Tasks.end()) {
            auto& pworkflow = TaskWorkflowDatas[itr->second->Workflow];
            auto tmpitr = std::find(pworkflow->Tasks.begin(), pworkflow->Tasks.end(), handle);
            if (tmpitr != pworkflow->Tasks.end()) {
                pworkflow->Tasks.erase(tmpitr);
            }
            Tasks.erase(itr);
        }
        return;
    }
}