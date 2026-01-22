#pragma once
#include "Task/TaskManager.h"
#include <TimeRecorder.h>
#include <concurrentqueue.h>
#include <unordered_map>

enum class ETaskType {
    TT_Timer,
    TT_Common,
    TT_Tick,
    TT_Cancelable,
};
typedef struct TaskDataBase_t {
    virtual ~TaskDataBase_t() {};
    TaskDataBase_t(ETaskType _Type,WorkflowHandle_t _Workflow) :
        Workflow(_Workflow), Type(_Type){
    }
    WorkflowHandle_t Workflow;
    ETaskType Type;
}TaskDataBase_t;

typedef struct TimerTaskData_t:TaskDataBase_t {
    TimerTaskData_t(WorkflowHandle_t _Workflow, TTimerTask _Task, std::chrono::nanoseconds _Repeat, std::chrono::nanoseconds _Timeout) :
        TaskDataBase_t(ETaskType::TT_Timer, _Workflow), Task(_Task), Repeat(_Repeat), Timeout(_Timeout){
    }
    TTimerTask Task;
    std::chrono::nanoseconds Repeat;
    std::chrono::nanoseconds Timeout;
}TimerTaskData_t;

typedef struct CommonTaskData_t :TaskDataBase_t {
    CommonTaskData_t( WorkflowHandle_t _Workflow,TCommonTask _Task):
        TaskDataBase_t(ETaskType::TT_Common, _Workflow),Task(_Task) {
    }
    TCommonTask Task;
}CommonTaskData_t;

typedef struct TickTaskData_t :TaskDataBase_t {
    TickTaskData_t(WorkflowHandle_t _Workflow, TTickTask _Task) :
        TaskDataBase_t(ETaskType::TT_Tick, _Workflow), Task(_Task) {
    }
    TTickTask Task;
}TickTaskData_t;

typedef struct CancelableTaskData_t :TaskDataBase_t {
    CancelableTaskData_t(WorkflowHandle_t _Workflow, FCancelableTaskBase* _Task) :
        TaskDataBase_t(ETaskType::TT_Cancelable, _Workflow), Task(_Task) {
    }
    FCancelableTaskBase* Task;
}CancelableTaskData_t;


typedef std::tuple<CommonHandle32_t, std::shared_ptr<TaskDataBase_t>> TAppendingTaskData;
typedef struct TaskWorkflow_t {
    std::chrono::nanoseconds RepeatTime{ DEFAULT_REPEAT_TIME };
    std::chrono::nanoseconds Timeout{ 0 };
    FTimeRecorder TimeRecorder;
    std::optional<std::future<void>> OptFuture;

    moodycamel::ConcurrentQueue<TAppendingTaskData> AppendingAddTaskDatas;
    moodycamel::ConcurrentQueue<TAppendingTaskData> AppendingDelTaskDatas;
    TAppendingTaskData AppendingTaskDataBuf[5];

    //thread read only
    std::unordered_map < CommonHandle32_t, std::shared_ptr<TickTaskData_t>> TickTasks;
    std::unordered_map < CommonHandle32_t, std::shared_ptr<TimerTaskData_t>> TimerTasks;
    std::unordered_map < CommonHandle32_t, std::shared_ptr<CommonTaskData_t>> Tasks;
    std::unordered_map < CommonHandle32_t, std::shared_ptr<CancelableTaskData_t>> CancelableTasks;
}TaskWorkflow_t;


class FTaskManagerBase : public ITaskManager {
public:
    FTaskManagerBase();
    ~FTaskManagerBase();

    WorkflowHandle_t GetMainThread() override;
    WorkflowHandle_t NewWorkflow() override;
    void ReleaseWorkflow(WorkflowHandle_t) override;
    void Run() override;
    void Tick() override;
    void Stop() override;

    CommonTaskHandle_t AddCancelableTask(WorkflowHandle_t handle, FCancelableTaskBase* task)override;
    CommonTaskHandle_t AddTick(WorkflowHandle_t, TTickTask task) override;
    CommonTaskHandle_t AddTimer(WorkflowHandle_t, TTimerTask task, uint64_t repeat, uint64_t timeout = 0) override;
    CommonTaskHandle_t AddTaskNoReturn(WorkflowHandle_t handle, TCommonTask task) override;
    void RemoveTask(CommonTaskHandle_t) override;

    void WorkflowThreadTick(std::shared_ptr<TaskWorkflow_t> pWorkflowData);
    void ThreadFinishTask(CommonHandle32_t,std::shared_ptr<TaskDataBase_t>);

    typedef std::tuple<WorkflowHandle_t, std::shared_ptr<TaskWorkflow_t>> TAppendingTaskWorkflowData;
    moodycamel::ConcurrentQueue<TAppendingTaskWorkflowData> AppendingAddTaskWorkflowDatas;
    TAppendingTaskWorkflowData AppendingAddTaskWorkflowDataBuf[5];
    moodycamel::ConcurrentQueue<WorkflowHandle_t> AppendingDelTaskWorkflowDatas;
    WorkflowHandle_t AppendingDelWorkflowHandleBuf[5];
    std::unordered_map<CommonHandle32_t, std::shared_ptr<TaskWorkflow_t>>  TaskWorkflowDatas;
    WorkflowHandle_t MainThread;


    typedef std::tuple<std::function<void()>> TSyncTaskData;
    moodycamel::ConcurrentQueue<TSyncTaskData> SyncAddTaskDatas;
    TSyncTaskData  SyncAddTaskDataBuf[100];
    typedef CommonTaskHandle_t TSyncTaskDelData;
    moodycamel::ConcurrentQueue<TSyncTaskDelData> SyncDelTaskDatas;
    TSyncTaskDelData SyncDelTaskDataBuf[100];
    typedef std::tuple<CommonHandle32_t, std::shared_ptr<TaskDataBase_t>> TSyncFinishedTaskData;
    moodycamel::ConcurrentQueue<TSyncFinishedTaskData> SyncFinishedTaskDatas;
    TSyncFinishedTaskData SyncFinishedTaskDataBuf[100];

    //main read only
    std::unordered_map < CommonHandle32_t, std::shared_ptr<TickTaskData_t>> TickTasks;
    std::unordered_map < CommonHandle32_t, std::shared_ptr<TimerTaskData_t>> TimerTasks;
    std::unordered_map < CommonHandle32_t, std::shared_ptr<CommonTaskData_t>> Tasks;
    std::unordered_map < CommonHandle32_t, std::shared_ptr<CancelableTaskData_t>> CancelableTasks;


    std::atomic_bool bRequestExit{ false };

};