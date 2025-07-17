#pragma once
#include <system_error>
#include <atomic>
#include <sqlpp23/sqlite3/sqlite3.h>
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
    bool bSent;
    std::error_code FinishCode;
    uint64_t SourceJobID{ 0 };
}SteamRequestHandle_t;

constexpr char SQL_FILE_NAME[] = "steamclient.db";
constexpr char SQL_CREATE_STEAM_USER_TABLE[] = R"(
CREATE TABLE  IF NOT EXISTS STEAMUSER(
SteamID             INT   PRIMARY KEY     NOT NULL,
AccountName         TEXT  UNIQUE, 
AccessToken         TEXT,
RefreshToken        TEXT
);
)";

constexpr char SQL_SELECT_FROM_STEAM_USER_BY_ACCOUNTNAME[] = R"(SELECT {} FROM STEAMUSER WHERE AccountName=?1 LIMIT 1;)";

constexpr char SQL_INSERT_STEAM_USER[] = R"(
INSERT OR REPLACE INTO STEAMUSER (SteamID,AccountName,AccessToken,RefreshToken)  
VALUES (?1, ?2, ?3, ?4); 
)";

constexpr char SQL_UPDATE_STEAM_USER_REFRESHTOEKN[] = R"(
UPDATE STEAMUSER set RefreshToken = ?1 where SteamID=?2;
)";

static int SqliteCB(void* NotUsed, int argc, char** argv, char** azColName) {
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}