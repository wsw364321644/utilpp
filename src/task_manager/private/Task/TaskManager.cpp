#include "Task/TaskManager.h"
#include "Task/TaskFlowTaskManager.h"

ITaskManager* GetTaskManagerSingleton()
{
    return FTaskFlowTaskManager::Get();
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
    RunningStatus = ERunningStatus::Null;
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