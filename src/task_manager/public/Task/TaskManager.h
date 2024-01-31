#include <functional>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <mutex>
#include <atomic>

#include <shared_mutex>
#include <handle.h>
#include <TimeRecorder.h>

constexpr std::chrono::nanoseconds DEFAULT_REPEAT_TIME = (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) / 60);


typedef std::function<void()> FCommonTask;
typedef std::function<void()> FTaskCompleteDelegate;
typedef std::function<void(float)> FTickTask;

typedef struct CommonTaskHandle_t : CommonHandle_t
{
    CommonTaskHandle_t(CommonHandle_t h) :CommonHandle_t(h) {}
    CommonTaskHandle_t() :CommonHandle_t() {}
    static std::atomic_uint32_t TaskCount;
}CommonTaskHandle_t;

typedef struct WorkflowHandle_t : CommonHandle_t
{
    constexpr WorkflowHandle_t(CommonHandle_t h) :CommonHandle_t(h) {}
    constexpr WorkflowHandle_t() : CommonHandle_t() {}
    static std::atomic_uint32_t WorkflowCount;
}WorkflowHandle_t;
inline constexpr WorkflowHandle_t RandomWorkflowHandle{ 0 };



typedef struct TimerTaskData_t {
    WorkflowHandle_t Workflow;
    FCommonTask Task;
    std::chrono::nanoseconds Repeat;
    std::chrono::nanoseconds Timeout;
}TimerTaskData_t;

typedef struct CommonTaskData_t {
    WorkflowHandle_t Workflow;
    FCommonTask Task;
    FTaskCompleteDelegate Delegate;
}CommonTaskData_t;

typedef struct TickTaskData_t {
    WorkflowHandle_t Workflow;
    FTickTask Task;
}TickTaskData_t;


struct TaskWorkflow_t;
typedef struct TaskWorkflow_t TaskWorkflow_t;

class FTaskManager {
public:
    static FTaskManager* Get();
    ~FTaskManager();

    WorkflowHandle_t NewWorkflow();
    // get MainThread handle
    WorkflowHandle_t GetMainThread();
    void ReleaseWorkflow(WorkflowHandle_t);
    void Run();
    void Tick();
    
    void Stop();

    CommonTaskHandle_t AddTick(WorkflowHandle_t, FTickTask task);
    CommonTaskHandle_t AddTimer(WorkflowHandle_t, FCommonTask task, uint64_t repeat, uint64_t timeout = 0);
    // pass a task to excuse
    CommonTaskHandle_t AddTask(WorkflowHandle_t, FCommonTask task, FTaskCompleteDelegate cb = nullptr);
    void RemoveTask(CommonTaskHandle_t);

    bool IsTaskComplete(CommonTaskHandle_t);
private:
    void TickTaskWorkflow(WorkflowHandle_t handle);
    FTaskManager();

    std::unordered_map<CommonHandle_t, std::shared_ptr<TaskWorkflow_t>>  TaskWorkflowDatas;
    std::shared_mutex  TaskWorkflowMutex;

    std::unordered_map < CommonHandle_t, std::shared_ptr<TickTaskData_t>> TickTasks;
    std::unordered_map < CommonHandle_t, std::shared_ptr<TimerTaskData_t>> TimerTasks;
    std::unordered_map < CommonHandle_t, std::shared_ptr<CommonTaskData_t>> Tasks;
    std::shared_mutex  TaskMutex;
    std::shared_mutex  TickMutex;
    std::shared_mutex  TimerMutex;

    void* InterData;
    std::atomic_bool bRequestExit{ false };
    WorkflowHandle_t MainThread;
};
