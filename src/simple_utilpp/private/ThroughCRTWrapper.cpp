#include "ThroughCRTWrapper.h"

ThroughCRTWrapper<std::shared_ptr<std::string>> TestGetString()
{
    auto out=std::make_shared<std::string>("dsfgsfg");
    return ThroughCRTWrapper<std::shared_ptr<std::string>>(out);
}
