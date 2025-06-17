#pragma once
#include <system_error>
#include <handle.h>
enum class ESteamClientError : int {
    SCE_OK = 0,
    SCE_InvalidInput,
    SCE_NotConnected,
    SCE_AlreadyLoggedin,
    SCE_RequestErrFromServer,
    SCE_RequestTimeout,
};
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
        case ESteamClientError::SCE_AlreadyLoggedin: return "Client Already Loggedin";
        default: return "Unknown error";
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
    bool bFinished{ true };
    uint64_t SourceJobID;

}SteamRequestHandle_t;
