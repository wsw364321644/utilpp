#pragma once
#include <system_error>
#include <std_ext.h>
#include "simple_error_def.h"

namespace utilpp {
    class common_used_error_category : public std::error_category {
    public:
        [[nodiscard]] virtual const char* name() const noexcept override {
            return "common_used_error";
        }

        [[nodiscard]] virtual std::string message(int _Errval) const override {
            auto ws_error = ECommonUsedError(_Errval);
            switch (ws_error)
            {
            case ECommonUsedError::CUE_OK: {
                return "OK";
            }
            default:
                break;
            }
            return "Unknown error";
        }
    };

    [[nodiscard]] inline const std::error_category& get_common_used_error_category() noexcept {
        thread_local common_used_error_category error_category_static;
        return error_category_static;
    }

    [[nodiscard]] inline const std::error_code make_common_used_error(ECommonUsedError err) noexcept {
        return std::error_code(std::to_underlying(err), get_common_used_error_category());
    }
}