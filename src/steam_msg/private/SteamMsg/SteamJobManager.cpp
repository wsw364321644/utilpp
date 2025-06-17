#include "SteamMsg/SteamJobManager.h"

bool FSteamJobManager::AddJob(FSteamGlobalID inID, FSteamJobCB inCB, FSteamJobFailedCB inFailedCB)
{
    auto[itr,bres]=Jobs.try_emplace(inID,SteamJobData_t{ .ID = inID ,.CB = inCB ,.FailedCB = inFailedCB });
    return bres;
}

void FSteamJobManager::JobHeartBeat(FSteamGlobalID inID)
{
}

void FSteamJobManager::JobFailed(FSteamGlobalID inID)
{
    auto itr=Jobs.find(inID);
    if (itr == Jobs.end()) {
        return;
    }
    auto& SteamJobData = itr->second;
    SteamJobData.FailedCB(ESteamClientError::SCE_RequestErrFromServer);
    Jobs.erase(itr);
}

void FSteamJobManager::CancelJob(FSteamGlobalID inID)
{
    auto itr = Jobs.find(inID);
    if (itr == Jobs.end()) {
        return;
    }
    Jobs.erase(itr);
}

void FSteamJobManager::JobCompleted(FSteamPacketMsg& msg, std::string_view bodyView)
{
    auto itr = Jobs.find(msg.GetTargetJobID());
    if (itr == Jobs.end()) {
        return;
    }
    auto& SteamJobData = itr->second;
    SteamJobData.CB(msg, bodyView);
    Jobs.erase(itr);
}
