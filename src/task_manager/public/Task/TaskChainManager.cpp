#include "TaskChainManager.h"

TASK_MANAGER_EXPORT FTaskChainManager* GetTaskChainManagerSingleton()
{
    return FTaskChainManager::GetSingleton()->GetSingleton().get();
}
