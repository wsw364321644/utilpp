#include "Task/TBBTaskManager.h"

FTBBTaskManager::FTBBTaskManager()
{

}

FTBBTaskManager::~FTBBTaskManager()
{
}

WorkflowHandle_t FTBBTaskManager::NewWorkflow()
{
    return NullHandle;
}

WorkflowHandle_t FTBBTaskManager::GetMainThread()
{
    return NullHandle;
}

void FTBBTaskManager::ReleaseWorkflow(WorkflowHandle_t)
{
}

void FTBBTaskManager::Run()
{
}

void FTBBTaskManager::Tick()
{
}

void FTBBTaskManager::Stop()
{
}
