#pragma once
#include <system_error>
enum class EWSError :int {
    WSE_OK = 0,
    WSE_INTERNEL_WSLIB_ERROR = 1,
    WSE_CONNECT_ERROR = 2,

};
class ws_error_category : public std::error_category {
public:
    [[nodiscard]] virtual const char* name() const noexcept override{
        return "wserror";
    }

    [[nodiscard]] virtual std::string message(int _Errval) const override {
        auto ws_error = EWSError(_Errval);
        switch (ws_error)
        {
        case EWSError::WSE_OK: {
            return "OK";
        }
        case EWSError::WSE_INTERNEL_WSLIB_ERROR: {
            return "Third part ws lib error";
        }
        case EWSError::WSE_CONNECT_ERROR: {
            return "Connection error";
        }
        default:
            break;
        }
        return "Unknown websocket error";
    }
};

[[nodiscard]] inline const std::error_category& ws_category() noexcept {
    static ws_error_category ws_error_category_static;
    return ws_error_category_static;
}