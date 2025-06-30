#pragma once
#include <system_error>
#include <atomic>
#include <handle.h>
#include <google/protobuf/service.h>
#include "SteamMsg/SteamClient.h"
class SteamClientErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return "SteamClientError";
    }

    std::string message(int ev) const override {
        switch (static_cast<ESteamClientError>(ev)) {
        case ESteamClientError::SCE_OK: return "OK";
        case ESteamClientError::SCE_InvalidInput: return "Invalid input";
        case ESteamClientError::SCE_NotConnected: return "Client not connected";
        case ESteamClientError::SCE_NotLogin: return "Client not login";
        case ESteamClientError::SCE_AlreadyLoggedin: return "Client Already Loggedin";
        case ESteamClientError::SCE_RequestErrFromServer: return "Client Request Err From Server";
        case ESteamClientError::SCE_RequestTimeout: return "Client Request Timeout";

        default: {
            if (ev < std::to_underlying(ESteamClientError::SCE_MAX)) {
                return "Error no mssage";
            }
            return "Unknown error";
        }
        }
    }
};

typedef struct SteamRequestHandle_t : ICommonHandle {
    SteamRequestHandle_t(uint64_t inSourceJobID) :SourceJobID(inSourceJobID), bFinished(false) {
    }
    SteamRequestHandle_t() = default;
    bool IsValid() const override {
        return !bFinished;
    }
    std::atomic_bool bFinished{ true };
    uint64_t SourceJobID{ 0 };
}SteamRequestHandle_t;
