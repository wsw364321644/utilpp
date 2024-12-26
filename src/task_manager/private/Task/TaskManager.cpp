#include "Task/TaskManager.h"
#include <taskflow/taskflow.hpp>
#include <TimeRecorder.h>
std::atomic_uint32_t WorkflowHandle_t::WorkflowCount;
std::atomic_uint32_t CommonTaskHandle_t::TaskCount;

typedef struct TaskWorkflow_t {
    std::chrono::nanoseconds RepeatTime{ DEFAULT_REPEAT_TIME };
    std::chrono::nanoseconds Timeout{ 0 };
    FTimeRecorder TimeRecorder;
    std::optional<std::future<void>> OptFuture;
    std::set<CommonTaskHandle_t> TimerTasks;
    std::set<CommonTaskHandle_t> TickTasks;
    std::set<CommonTaskHandle_t> Tasks;
    std::set<CommonTaskHandle_t> CancelableTasks;
}TaskWorkflow_t;

typedef struct TimerTaskData_t {
    WorkflowHandle_t Workflow;
    TTimerTask Task;
    std::chrono::nanoseconds Repeat;
    std::chrono::nanoseconds Timeout;
}TimerTaskData_t;

typedef struct CommonTaskData_t {
    WorkflowHandle_t Workflow;
    TCommonTask Task;
}CommonTaskData_t;

typedef struct TickTaskData_t {
    WorkflowHandle_t Workflow;
    TTickTask Task;
}TickTaskData_t;

typedef struct CancelableTaskData_t {
    WorkflowHandle_t Workflow;
    FCancelableTaskBase* Task;
}CancelableTaskData_t;

class FTaskManagerBase : public ITaskManager {
public:
    FTaskManagerBase();
    ~FTaskManagerBase();

    WorkflowHandle_t NewWorkflow() override;
    WorkflowHandle_t GetMainThread() override;
    void ReleaseWorkflow(WorkflowHandle_t) override;
    void Run() override;
    void Tick() override;
    void Stop() override;

    CommonTaskHandle_t AddCancelableTask(WorkflowHandle_t handle, FCancelableTaskBase* task)override;
    CommonTaskHandle_t AddTick(WorkflowHandle_t, TTickTask task) override;
    CommonTaskHandle_t AddTimer(WorkflowHandle_t, TTimerTask task, uint64_t repeat, uint64_t timeout = 0) override;
    CommonTaskHandle_t AddTaskNoReturn(WorkflowHandle_t handle, TCommonTask task) override;
    void RemoveTask(CommonTaskHandle_t) override;


    

    std::unordered_map<CommonHandle_t, std::shared_ptr<TaskWorkflow_t>>  TaskWorkflowDatas;
    std::shared_mutex  TaskWorkflowMutex;

    std::unordered_map < CommonHandle_t, std::shared_ptr<TickTaskData_t>> TickTasks;
    std::unordered_map < CommonHandle_t, std::shared_ptr<TimerTaskData_t>> TimerTasks;
    std::unordered_map < CommonHandle_t, std::shared_ptr<CommonTaskData_t>> Tasks;
    std::unordered_map < CommonHandle_t, std::shared_ptr<CancelableTaskData_t>> CancelableTasks;
    std::shared_mutex  TaskMutex;
    std::shared_mutex  TickMutex;
    std::shared_mutex  TimerMutex;
    std::shared_mutex  CancelableTaskMutex;

    std::atomic_bool bRequestExit{ false };
    WorkflowHandle_t MainThread;
};


FTaskManagerBase::FTaskManagerBase() {
    TaskWorkflowDatas.try_emplace(NullHandle, std::make_shared<TaskWorkflow_t>());
    MainThread = NewWorkflow();
}

FTaskManagerBase::~FTaskManagerBase()
{
}




WorkflowHandle_t FTaskManagerBase::NewWorkflow()
{
    std::scoped_lock lock(TaskWorkflowMutex);
    auto pair = TaskWorkflowDatas.try_emplace(WorkflowHandle_t::WorkflowCount, std::make_shared<TaskWorkflow_t>());
    if (!pair.second) {
        return NullHandle;
    }
    return pair.first->first;
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
    std::scoped_lock lock(TaskWorkflowMutex);
    TaskWorkflowDatas.erase(handle);
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

}

void FTaskManagerBase::Stop()
{
    bRequestExit = true;
}


CommonTaskHandle_t FTaskManagerBase::AddCancelableTask(WorkflowHandle_t handle, FCancelableTaskBase* task)
{
    std::shared_lock TaskWorkflowRLock{ TaskWorkflowMutex, std::defer_lock };
    std::unique_lock CancelableTaskLock{ CancelableTaskMutex, std::defer_lock };
    std::scoped_lock lock(CancelableTaskLock, TaskWorkflowRLock);

    auto itr = TaskWorkflowDatas.find(handle);
    if (itr == TaskWorkflowDatas.end()) {
        return NullHandle;
    }
    std::shared_ptr<TaskWorkflow_t> pTaskWorkflow = itr->second;

    auto res = CancelableTasks.emplace(CommonTaskHandle_t::TaskCount, std::make_shared<CancelableTaskData_t>(handle, task));
    if (!res.second) {
        return NullHandle;
    }
    auto setRes =pTaskWorkflow->CancelableTasks.emplace(res.first->first);
    if (!setRes.second) {
        CancelableTasks.erase(res.first->first);
        return NullHandle;
    }
    task->Init(res.first->first);
    return res.first->first;
}

CommonTaskHandle_t FTaskManagerBase::AddTick(WorkflowHandle_t handle, TTickTask task)
{
    std::shared_lock TaskWorkflowRLock{ TaskWorkflowMutex, std::defer_lock };
    std::unique_lock TickLock{ TickMutex, std::defer_lock };
    std::scoped_lock lock(TaskWorkflowRLock, TickLock);
    auto itr = TaskWorkflowDatas.find(handle);
    if (itr == TaskWorkflowDatas.end()) {
        return NullHandle;
    }
    auto res = TickTasks.emplace(CommonTaskHandle_t::TaskCount, std::make_shared<TickTaskData_t>(handle, task));
    if (!res.second) {
        return NullHandle;
    }
    auto setRes = itr->second->TickTasks.emplace(res.first->first);
    if (!setRes.second) {
        TickTasks.erase(res.first->first);
        return NullHandle;
    }
    return res.first->first;
}

CommonTaskHandle_t FTaskManagerBase::AddTimer(WorkflowHandle_t handle, TTimerTask task, uint64_t repeat, uint64_t timeout)
{
    std::shared_lock TaskWorkflowRLock{ TaskWorkflowMutex, std::defer_lock };
    std::unique_lock TimerLock{ TimerMutex, std::defer_lock };
    std::scoped_lock lock(TaskWorkflowRLock, TimerLock);
    auto itr = TaskWorkflowDatas.find(handle);
    if (itr == TaskWorkflowDatas.end()) {
        return NullHandle;
    }
    auto res = TimerTasks.emplace(CommonTaskHandle_t::TaskCount, std::make_shared < TimerTaskData_t>(handle, task, std::chrono::nanoseconds(repeat), std::chrono::nanoseconds(timeout)));
    if (!res.second) {
        return NullHandle;
    }
    auto setRes = itr->second->TimerTasks.emplace(res.first->first);
    if (!setRes.second) {
        TimerTasks.erase(res.first->first);
        return NullHandle;
    }
    return res.first->first;
}

CommonTaskHandle_t FTaskManagerBase::AddTaskNoReturn(WorkflowHandle_t handle, TCommonTask task) {
    std::shared_lock TaskWorkflowRLock{ TaskWorkflowMutex, std::defer_lock };
    std::unique_lock TaskLock{ TaskMutex, std::defer_lock };
    std::scoped_lock lock(TaskWorkflowRLock, TaskLock);
    auto itr = TaskWorkflowDatas.find(handle);
    if (itr == TaskWorkflowDatas.end()) {
        return NullHandle;
    }
    auto res = Tasks.emplace(CommonTaskHandle_t::TaskCount, std::make_shared < CommonTaskData_t>(handle,task));
    if (!res.second) {
        return NullHandle;
    }
    auto setRes = itr->second->Tasks.emplace(res.first->first);
    if (!setRes.second) {
        Tasks.erase(res.first->first);
        return NullHandle;
    }
    return res.first->first;
}

void FTaskManagerBase::RemoveTask(CommonTaskHandle_t handle)
{
    std::shared_lock TaskWorkflowLock{ TaskWorkflowMutex, std::defer_lock };
    std::unique_lock TaskLock{ TaskMutex, std::defer_lock };
    std::unique_lock TickLock{ TickMutex, std::defer_lock };
    std::unique_lock TimerLock{ TimerMutex, std::defer_lock };
    std::unique_lock CancelableLock{ CancelableTaskMutex, std::defer_lock };
    {
        std::scoped_lock lock(TaskWorkflowLock, TickMutex);
        auto itr = TickTasks.find(handle);
        if (itr != TickTasks.end()) {
            auto pair = TaskWorkflowDatas.find(itr->second->Workflow);
            if (pair != TaskWorkflowDatas.end()) {
                auto& pworkflow = pair->second;
                auto tmpitr = std::find(pworkflow->TickTasks.begin(), pworkflow->TickTasks.end(), handle);
                if (tmpitr != pworkflow->TickTasks.end()) {
                    pworkflow->TickTasks.erase(tmpitr);
                }
            }
            TickTasks.erase(itr);
            return;
        }
    }
    {
        std::scoped_lock lock(TaskWorkflowLock, TimerMutex);
        auto itr = TimerTasks.find(handle);
        if (itr != TimerTasks.end()) {
            auto pair = TaskWorkflowDatas.find(itr->second->Workflow);
            if (pair != TaskWorkflowDatas.end()) {
                auto& pworkflow = pair->second;
                auto tmpitr = std::find(pworkflow->TimerTasks.begin(), pworkflow->TimerTasks.end(), handle);
                if (tmpitr != pworkflow->TimerTasks.end()) {
                    pworkflow->TimerTasks.erase(tmpitr);
                }
            }
            TimerTasks.erase(itr);
            return;
        }
    }
    {
        std::scoped_lock lock(TaskWorkflowLock, TaskLock);
        auto itr = Tasks.find(handle);
        if (itr != Tasks.end()) {
            auto pair = TaskWorkflowDatas.find(itr->second->Workflow);
            if (pair != TaskWorkflowDatas.end()) {
                auto& pworkflow = pair->second;
                auto tmpitr = std::find(pworkflow->Tasks.begin(), pworkflow->Tasks.end(), handle);
                if (tmpitr != pworkflow->Tasks.end()) {
                    pworkflow->Tasks.erase(tmpitr);
                }
            }
            Tasks.erase(itr);
            return;
        }
    }
    {
        std::shared_ptr<CancelableTaskData_t> pCancelableTaskData;
        {
            std::scoped_lock lock(TaskWorkflowLock, CancelableLock);
            auto itr = CancelableTasks.find(handle);
            if (itr != CancelableTasks.end()) {
                auto pair = TaskWorkflowDatas.find(itr->second->Workflow);
                if (pair != TaskWorkflowDatas.end()) {
                    auto& pworkflow = pair->second;
                    auto& taskVec = pworkflow->CancelableTasks;
                    auto tmpitr = std::find(taskVec.begin(), taskVec.end(), handle);
                    if (tmpitr != taskVec.end()) {
                        taskVec.erase(tmpitr);
                    }
                }
                pCancelableTaskData = itr->second;
                CancelableTasks.erase(itr);
            }
        }
        if (pCancelableTaskData) {

            WorkflowHandle_t FinalizeWorkflow{ pCancelableTaskData->Workflow };
            if (pCancelableTaskData->Task->FinalizeWorkflow.IsValid()) {
                FinalizeWorkflow = pCancelableTaskData->Task->FinalizeWorkflow;
            }
            AddTask(FinalizeWorkflow, [pCancelableTaskData]() {
                pCancelableTaskData->Task->OnStop();
                pCancelableTaskData->Task->Finalize();
                });
            return;
        }
    }
}

class  FTaskManager :public FTaskManagerBase {
public:
    static FTaskManager* Get();
    void Tick() override;
private:
    FTaskManager();
    void TickTaskWorkflow(WorkflowHandle_t handle);
    //void TickRandomTaskWorkflow();

    tf::Executor Executor;
};
FTaskManager::FTaskManager() {

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

void FTaskManager::Tick() {
    std::shared_lock TaskWorkflowLock{ TaskWorkflowMutex, std::defer_lock };
    std::unique_lock TaskLock{ TaskMutex, std::defer_lock };
    std::shared_lock TaskRLock{ TaskMutex, std::defer_lock };
    TaskWorkflowLock.lock();
    for (auto& pair : TaskWorkflowDatas) {
        auto& pTaskWorkflow = pair.second;
        auto handle = pair.first;
        if (handle == MainThread)
        {
            TickTaskWorkflow(handle);
        }
        else if (handle == NullHandle) {
            auto& optfuture = pTaskWorkflow->OptFuture;
            if (optfuture.has_value() && optfuture.value().wait_for(std::chrono::seconds(0)) == std::future_status::timeout) {
                continue;
            }
            optfuture = Executor.async("", [handle, this]() {
                TickTaskWorkflow(handle);
                });
        }
        else {
            auto& optfuture = pTaskWorkflow->OptFuture;
            if (optfuture.has_value() && optfuture.value().wait_for(std::chrono::seconds(0)) == std::future_status::timeout) {
                continue;
            }
            optfuture = Executor.async("", [handle, this]() {
                TickTaskWorkflow(handle);
                });
        }
    }
}

void FTaskManager::TickTaskWorkflow(WorkflowHandle_t handle)
{
    std::shared_lock TaskWorkflowLock{ TaskWorkflowMutex, std::defer_lock };
    std::shared_lock TickRLock{ TickMutex, std::defer_lock };
    std::shared_lock TimerRLock{ TimerMutex, std::defer_lock };
    std::unique_lock TaskLock{ TaskMutex, std::defer_lock };
    std::shared_lock TaskRLock{ TaskMutex, std::defer_lock };
    std::unique_lock CancelableTaskLock{ CancelableTaskMutex, std::defer_lock };
    std::shared_lock CancelableTaskRLock{ CancelableTaskMutex, std::defer_lock };

    tf::Taskflow Taskflow;
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
        std::set<CommonTaskHandle_t> localThreadTickTasks;
        {
            std::scoped_lock lock(TickRLock);
            localTickTasks = TickTasks;
            localThreadTickTasks = pTaskWorkflow->TickTasks;
        }
        for (auto& task : localThreadTickTasks) {
            if (handle != NullHandle) {
                localTickTasks[task]->Task(delsec);
            }
            else {
                Taskflow.emplace(
                    [task = localTickTasks[task]->Task, delsec]() {
                        task(delsec);
                    }
                );
            }
        }
    }

    {
        std::unordered_map < CommonHandle_t, std::shared_ptr<TimerTaskData_t>> localTimerTasks;
        std::set<CommonTaskHandle_t> localThreadTasks;
        {
            std::scoped_lock lock(TimerRLock);
            localTimerTasks = TimerTasks;
            localThreadTasks = pTaskWorkflow->TimerTasks;
        }
        for (auto& handle : localThreadTasks) {
            if (localTimerTasks[handle]->Timeout <= deltime) {
                localTimerTasks[handle]->Timeout = localTimerTasks[handle]->Repeat;
                if (handle != NullHandle) {
                    localTimerTasks[handle]->Task(handle);
                }else{
                    Taskflow.emplace(
                        [task = localTimerTasks[handle]->Task, handle]() {
                            task(handle);
                        }
                    );
                }
            }
            else {
                localTimerTasks[handle]->Timeout -= deltime;
            }
        }
    }


    if (needTick) {
        std::set<CommonTaskHandle_t> localCancelableTasks;
        std::unordered_map < CommonHandle_t, std::shared_ptr<CancelableTaskData_t>> localCancelableTaskDatas;
        {
            std::scoped_lock lock(TaskRLock);
            localCancelableTasks = pTaskWorkflow->CancelableTasks;
            localCancelableTaskDatas = CancelableTasks;
        }
        for (auto& task : localCancelableTasks) {
            std::shared_ptr<CancelableTaskData_t> pTaskData;
            pTaskData = localCancelableTaskDatas[task];
            if (handle != NullHandle) {
                pTaskData->Task->Tick(delsec);
            }
            else {
                Taskflow.emplace(
                    [task = pTaskData->Task, delsec]() {
                        task->Tick(delsec);
                    }
                );
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
            if (handle != NullHandle) {
                pTaskData->Task();
            }
            else {
                Taskflow.emplace(
                    [task = pTaskData->Task]() {
                        task();
                    }
                );
            }
        }
    }



    if (handle == NullHandle) {
        Executor.run(Taskflow).wait();
    }
    {
        std::scoped_lock lock(TaskLock);
        for (auto& task : localTasks) {
            Tasks.erase(task);
            pTaskWorkflow->Tasks.erase(task);
        }
    }
    
}

//void FTaskManager::TickRandomTaskWorkflow()
//{
//    std::shared_lock TaskWorkflowLock{ TaskWorkflowMutex, std::defer_lock };
//    std::unique_lock TaskLock{ TaskMutex, std::defer_lock };
//    std::shared_lock TaskRLock{ TaskMutex, std::defer_lock };
//    std::shared_lock TickRLock{ TickMutex, std::defer_lock };
//    std::shared_lock TimerRLock{ TimerMutex, std::defer_lock };
//
//    tf::Taskflow Taskflow;
//    std::shared_ptr<TaskWorkflow_t> pTaskWorkflow;
//    {
//        std::scoped_lock lock(TaskWorkflowLock);
//        pTaskWorkflow = TaskWorkflowDatas[NullHandle];
//    }
//    pTaskWorkflow->TimeRecorder.Tick();
//    auto deltime = pTaskWorkflow->TimeRecorder.GetDelta<std::chrono::nanoseconds>();
//    auto delsec = float(deltime.count()) / std::chrono::nanoseconds::period::den;
//    bool needTick;
//    if (pTaskWorkflow->Timeout <= deltime) {
//        pTaskWorkflow->Timeout = pTaskWorkflow->RepeatTime;
//        needTick = true;
//    }
//    else {
//        pTaskWorkflow->Timeout -= deltime;
//        needTick = false;
//    }
//
//    if (needTick) {
//        std::unordered_map < CommonHandle_t, std::shared_ptr<TickTaskData_t>> localTickTasks;
//        std::set<CommonTaskHandle_t> localThreadTickTasks;
//        {
//            std::scoped_lock lock(TickRLock);
//            localTickTasks = TickTasks;
//            localThreadTickTasks = pTaskWorkflow->TickTasks;
//        }
//        for (auto& task : localThreadTickTasks) {
//            Taskflow.emplace(
//                [task = localTickTasks[task]->Task, delsec]() {
//                    task(delsec);
//                }
//            );
//        }
//    }
//
//    {
//        std::unordered_map < CommonHandle_t, std::shared_ptr<TimerTaskData_t>> localTimerTasks;
//        std::set<CommonTaskHandle_t> localThreadTasks;
//        {
//            std::scoped_lock lock(TimerRLock);
//            localTimerTasks = TimerTasks;
//            localThreadTasks = pTaskWorkflow->TimerTasks;
//        }
//        for (auto& handle : localThreadTasks) {
//            if (localTimerTasks[handle]->Timeout <= deltime) {
//                localTimerTasks[handle]->Timeout = localTimerTasks[handle]->Repeat;
//                Taskflow.emplace(
//                    [task = localTimerTasks[handle]->Task, handle]() {
//                        task(handle);
//                    }
//                );
//            }
//            else {
//                localTimerTasks[handle]->Timeout -= deltime;
//            }
//        }
//    }
//
//    std::set<CommonTaskHandle_t> localTasks;
//    {
//        std::unordered_map < CommonHandle_t, std::shared_ptr<CommonTaskData_t>> localTaskDatas;
//        {
//            std::scoped_lock lock(TaskRLock);
//            localTasks = pTaskWorkflow->Tasks;
//            localTaskDatas = Tasks;
//        }
//        for (auto& task : localTasks) {
//            std::shared_ptr<CommonTaskData_t> pTaskData;
//            pTaskData = localTaskDatas[task];
//            Taskflow.emplace(
//                [task = pTaskData->Task]() {
//                    task();
//                }
//            );
//        }
//    }
//    Executor.run(Taskflow).wait();
//    {
//        {
//            std::scoped_lock lock(TaskLock);
//            for (auto& task : localTasks) {
//                Tasks.erase(task);
//                pTaskWorkflow->Tasks.erase(task);
//            }
//        }
//    }
//}


ITaskManager* GetTaskManagerInstance()
{
    return FTaskManager::Get();
}

FCancelableTaskBase::~FCancelableTaskBase()
{
}

void FCancelableTaskBase::Finalize()
{
    RunningStatus = ERunningStatus::Finished;
}

void FCancelableTaskBase::Reset()
{
    FinalizeWorkflow = NullHandle;
    Manager = nullptr;
    RunningStatus=ERunningStatus::Null;
}

void FCancelableTaskBase::Init(CommonTaskHandle_t inSelfHandle)
{
    SelfHandle = inSelfHandle;
    RunningStatus = ERunningStatus::Running;
}

void FCancelableTaskBase::SetFinalizeWorkflow(WorkflowHandle_t Workflow)
{
    FinalizeWorkflow = Workflow;
}

//void FCancelableTaskTempWrapper::Tick(float delta)
//{
//    Task->Tick(delta);
//}
//
//void FCancelableTaskTempWrapper::Init()
//{
//    super::Init();
//    Task->Init();
//}
//
//void FCancelableTaskTempWrapper::Finalize()
//{
//    super::Finalize();
//    Task->Finalize();
//    ReleaseFunc();
//}

void FCancelableTaskTemp::Finalize()
{
    super::Finalize();
    if (ReleaseFunc) {
        ReleaseFunc();
    }
}
void FCancelableTaskTemp::SetReleaseFunc(const std::function<void()> InFunc)
{
    ReleaseFunc = InFunc;
}