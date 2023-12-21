#include "HTTP/HttpManager.h"
std::unordered_map<std::string, std::function<HttpManagerPtr()>> FHttpManager::FnCreates;