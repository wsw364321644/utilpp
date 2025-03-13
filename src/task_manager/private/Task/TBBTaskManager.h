#pragma once
#include "Task/TaskManager.h"
#include <TimeRecorder.h>

class FTBBTaskManager : public ITaskManager {
public:
    FTBBTaskManager();
    ~FTBBTaskManager();

    WorkflowHandle_t NewWorkflow() override;
    WorkflowHandle_t GetMainThread() override;
    void ReleaseWorkflow(WorkflowHandle_t) override;
    void Run() override;
    void Tick() override;
    void Stop() override;


};