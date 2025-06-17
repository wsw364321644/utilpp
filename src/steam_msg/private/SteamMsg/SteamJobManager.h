#pragma once
#include <stdint.h>
#include <SteamTypes.h>
#include <SteamLanguage.h>
#include <unordered_map>
#include <functional>
#include <steam/steammessages_base.pb.h>
#include "SteamMsg/SteamPacketMessage.h"
#include "SteamMsg/SteamClientInternal.h"

typedef std::function<void(FSteamPacketMsg&,std::string_view)> FSteamJobCB;
typedef std::function<void(ESteamClientError)> FSteamJobFailedCB;
typedef struct SteamJobData_t {
    FSteamGlobalID ID;
    FSteamJobCB CB;
    FSteamJobFailedCB FailedCB;
}SteamJobData_t;
class FSteamJobManager {
public:
    bool AddJob(FSteamGlobalID inID, FSteamJobCB inCB, FSteamJobFailedCB inFailedCB);
    void JobHeartBeat(FSteamGlobalID inID);
    void JobFailed(FSteamGlobalID inID);
    void CancelJob(FSteamGlobalID inID);
    void JobCompleted(FSteamPacketMsg&, std::string_view);
    std::unordered_map<FSteamGlobalID, SteamJobData_t> Jobs;
};