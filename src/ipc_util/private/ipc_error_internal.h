#pragma once
#include "ipc_error.h"
#include <system_error>
namespace utilpp{
    class ipc_error_category : public std::error_category {
    public:
        [[nodiscard]] virtual const char* name() const noexcept override {
            return "ipc_error";
        }

        [[nodiscard]] virtual std::string message(int _Errval) const override {
            auto ws_error = EIPCError(_Errval);
            switch (ws_error)
            {
            case EIPCError::IPCE_OK: {
                return "OK";
            }
            case EIPCError::IPCE_AlreadyExist: {
                return "Already Exist";
            }
            default:
                break;
            }
            return "Unknown error";
        }
    };

    [[nodiscard]] inline const std::error_category& get_ipc_error_category() noexcept {
        thread_local ipc_error_category ws_error_category_static;
        return ws_error_category_static;
    }

    [[nodiscard]] inline const std::error_code make_ipc_error(EIPCError err) noexcept {
        return std::error_code(std::to_underlying(err), get_ipc_error_category());
    }
}