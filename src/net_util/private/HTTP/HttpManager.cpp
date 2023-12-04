#include "HTTP/HttpManager.h"

HttpRequestPtr FHttpManager::NewRequest()
{
    return FnCreate();
}

