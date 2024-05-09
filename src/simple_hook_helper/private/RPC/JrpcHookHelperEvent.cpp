#include "RPC/JrpcHookHelperEvent.h"
#include "jrpc_parser.h"
#include <nlohmann/json.hpp>
#include <mbedtls/base64.h>
#include <RPC/message_common.h>
#include <gcem.hpp>

constexpr int MOUSE_WHEEL_EVENT_BASE64_LEN = (sizeof(mouse_wheel_event_t) + 2) / 3 * 4;
constexpr int MOUSE_BOTTON_EVENT_BASE64_LEN = (sizeof(mouse_button_event_t) + 2) / 3 * 4;
constexpr int MOUSE_MOTION_EVENT_BASE64_LEN = (sizeof(mouse_motion_event_t) + 2) / 3 * 4;
constexpr int KEYBOARD_EVENT_BASE64_LEN = (sizeof(keyboard_event_t) + 2) / 3 * 4;

DEFINE_RPC_OVERRIDE_FUNCTION(JRPCHookHelperEventAPI, "HookHelperEvent");
DEFINE_JRPC_OVERRIDE_FUNCTION(JRPCHookHelperEventAPI);


REGISTER_RPC_EVENT_API_AUTO(JRPCHookHelperEventAPI, HotkeyListUpdate);
DEFINE_REQUEST_RPC_EVENT(JRPCHookHelperEventAPI, HotkeyListUpdate);
bool JRPCHookHelperEventAPI::HotkeyListUpdate(HotKeyList_t& HotKeyList)
{
    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->Method = HotkeyListUpdateName;
    nlohmann::json obj = nlohmann::json::object();
    for (auto pair: HotKeyList) {
        for (auto hotkey : pair.second) {
            nlohmann::json hotkeyNode;
            hotkeyNode["mod"] = hotkey.mod;
            hotkeyNode["keyCode"] = hotkey.key_code;
            obj[pair.first].push_back(hotkeyNode);
        }
    }

    req->Params = obj.dump();
    return  processer->SendEvent(req);
}

void JRPCHookHelperEventAPI::OnHotkeyListUpdateRequestRecv(std::shared_ptr<RPCRequest> req)
{
    std::shared_ptr<JsonRPCRequest> jreq = std::dynamic_pointer_cast<JsonRPCRequest>(req);
    auto doc = jreq->GetParamsNlohmannJson();
    HotKeyList_t HotKeyList;
    if (!recvHotkeyListUpdateDelegate) {
        return;
    }
    for (auto it = doc.begin(); it != doc.end();it++) {
        for (auto hotkeyNode : it.value()) {
            key_with_modifier_t key{.key_code= (SDL_Keycode)hotkeyNode["keyCode"].get_ref<nlohmann::json::number_integer_t&>(),
                .mod=(Uint16) hotkeyNode["mod"].get_ref<nlohmann::json::number_integer_t&>()};
            HotKeyList[it.key()].push_back(key);
        }
    }
    recvHotkeyListUpdateDelegate(HotKeyList);
}



REGISTER_RPC_EVENT_API_AUTO(JRPCHookHelperEventAPI, OverlayMouseWheelEvent);
DEFINE_REQUEST_RPC_EVENT(JRPCHookHelperEventAPI, OverlayMouseWheelEvent);
bool JRPCHookHelperEventAPI::OverlayMouseWheelEvent(uint64_t windowId, mouse_wheel_event_t& event)
{
    uint8_t base64buf[MOUSE_WHEEL_EVENT_BASE64_LEN+1];
    size_t olen;
    int res=mbedtls_base64_encode(base64buf, MOUSE_WHEEL_EVENT_BASE64_LEN, &olen,(const uint8_t*) & event, sizeof(mouse_wheel_event_t));
    if (res != 0) {
        return false;
    }
    base64buf[olen] = 0;

    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->Method = OverlayMouseWheelEventName;
    nlohmann::json obj = nlohmann::json::object();
    obj["windowId"] = windowId;
    obj["event"] = base64buf;
    req->Params = obj.dump();
    return  processer->SendEvent(req);
}

void JRPCHookHelperEventAPI::OnOverlayMouseWheelEventRequestRecv(std::shared_ptr<RPCRequest> req)
{
    std::shared_ptr<JsonRPCRequest> jreq = std::dynamic_pointer_cast<JsonRPCRequest>(req);
    auto doc = jreq->GetParamsNlohmannJson();
    if (!recvOverlayMouseWheelEventDelegate) {
        return;
    }
    mouse_wheel_event_t outEvent;
    size_t olen;
    auto base64_cstr = doc["event"].get_ref<nlohmann::json::string_t&>().c_str();
    int res = mbedtls_base64_decode((uint8_t*)&outEvent, sizeof(mouse_wheel_event_t), &olen, (const uint8_t*)base64_cstr, doc["event"].get_ref<nlohmann::json::string_t&>().size());
    if (res != 0) {
        return;
    }
    recvOverlayMouseWheelEventDelegate(doc["windowId"].get_ref<nlohmann::json::number_integer_t&>(), outEvent);
}



REGISTER_RPC_EVENT_API_AUTO(JRPCHookHelperEventAPI, OverlayMouseButtonEvent);
DEFINE_REQUEST_RPC_EVENT(JRPCHookHelperEventAPI, OverlayMouseButtonEvent);
bool JRPCHookHelperEventAPI::OverlayMouseButtonEvent(uint64_t windowId, mouse_button_event_t& event)
{
    uint8_t base64buf[MOUSE_WHEEL_EVENT_BASE64_LEN + 1];
    size_t olen;
    int res = mbedtls_base64_encode(base64buf, MOUSE_WHEEL_EVENT_BASE64_LEN, &olen, (const uint8_t*)&event, sizeof(mouse_button_event_t));
    if (res != 0) {
        return false;
    }
    base64buf[olen] = 0;

    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->Method = OverlayMouseButtonEventName;
    nlohmann::json obj = nlohmann::json::object();
    obj["windowId"] = windowId;
    obj["event"] = base64buf;


    req->Params = obj.dump();
    return  processer->SendEvent(req);
}

void JRPCHookHelperEventAPI::OnOverlayMouseButtonEventRequestRecv(std::shared_ptr<RPCRequest> req)
{
    std::shared_ptr<JsonRPCRequest> jreq = std::dynamic_pointer_cast<JsonRPCRequest>(req);
    auto doc = jreq->GetParamsNlohmannJson();
    if (!recvOverlayMouseButtonEventDelegate) {
        return;
    }
    mouse_button_event_t outEvent;
    size_t olen;
    auto base64_cstr = doc["event"].get_ref<nlohmann::json::string_t&>().c_str();
    int res = mbedtls_base64_decode((uint8_t*)&outEvent, sizeof(mouse_button_event_t), &olen, (const uint8_t*)base64_cstr, doc["event"].get_ref<nlohmann::json::string_t&>().size());
    if (res != 0) {
        return;
    }
    recvOverlayMouseButtonEventDelegate(doc["windowId"].get_ref<nlohmann::json::number_integer_t&>(), outEvent);
}


REGISTER_RPC_EVENT_API_AUTO(JRPCHookHelperEventAPI, OverlayMouseMotionEvent);
DEFINE_REQUEST_RPC_EVENT(JRPCHookHelperEventAPI, OverlayMouseMotionEvent);
bool JRPCHookHelperEventAPI::OverlayMouseMotionEvent(uint64_t windowId, mouse_motion_event_t& event)
{
    uint8_t base64buf[MOUSE_WHEEL_EVENT_BASE64_LEN + 1];
    size_t olen;
    int res = mbedtls_base64_encode(base64buf, MOUSE_WHEEL_EVENT_BASE64_LEN, &olen, (const uint8_t*)&event, sizeof(mouse_motion_event_t));
    if (res != 0) {
        return false;
    }
    base64buf[olen] = 0;

    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->Method = OverlayMouseMotionEventName;
    nlohmann::json obj = nlohmann::json::object();
    obj["windowId"] = windowId;
    obj["event"] = base64buf;


    req->Params = obj.dump();
    return  processer->SendEvent(req);
}

void JRPCHookHelperEventAPI::OnOverlayMouseMotionEventRequestRecv(std::shared_ptr<RPCRequest> req)
{
    std::shared_ptr<JsonRPCRequest> jreq = std::dynamic_pointer_cast<JsonRPCRequest>(req);
    auto doc = jreq->GetParamsNlohmannJson();
    if (!recvOverlayMouseMotionEventDelegate) {
        return;
    }
    mouse_motion_event_t outEvent;
    size_t olen;
    auto base64_cstr = doc["event"].get_ref<nlohmann::json::string_t&>().c_str();
    int res = mbedtls_base64_decode((uint8_t*)&outEvent, sizeof(mouse_motion_event_t), &olen, (const uint8_t*)base64_cstr, doc["event"].get_ref<nlohmann::json::string_t&>().size());
    if (res != 0) {
        return;
    }
    recvOverlayMouseMotionEventDelegate(doc["windowId"].get_ref<nlohmann::json::number_integer_t&>(), outEvent);
}



REGISTER_RPC_EVENT_API_AUTO(JRPCHookHelperEventAPI, OverlayKeyboardEvent);
DEFINE_REQUEST_RPC_EVENT(JRPCHookHelperEventAPI, OverlayKeyboardEvent);
bool JRPCHookHelperEventAPI::OverlayKeyboardEvent(uint64_t windowId, keyboard_event_t& event)
{
    uint8_t base64buf[MOUSE_WHEEL_EVENT_BASE64_LEN + 1];
    size_t olen;
    int res = mbedtls_base64_encode(base64buf, MOUSE_WHEEL_EVENT_BASE64_LEN, &olen, (const uint8_t*)&event, sizeof(keyboard_event_t));
    if (res != 0) {
        return false;
    }
    base64buf[olen] = 0;

    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->Method = OverlayKeyboardEventName;
    nlohmann::json obj = nlohmann::json::object();
    obj["windowId"] = windowId;
    obj["event"] = base64buf;


    req->Params = obj.dump();
    return  processer->SendEvent(req);
}

void JRPCHookHelperEventAPI::OnOverlayKeyboardEventRequestRecv(std::shared_ptr<RPCRequest> req)
{
    std::shared_ptr<JsonRPCRequest> jreq = std::dynamic_pointer_cast<JsonRPCRequest>(req);
    auto doc = jreq->GetParamsNlohmannJson();
    if (!recvOverlayKeyboardEventDelegate) {
        return;
    }
    keyboard_event_t outEvent;
    size_t olen;
    auto base64_cstr = doc["event"].get_ref<nlohmann::json::string_t&>().c_str();
    int res = mbedtls_base64_decode((uint8_t*)&outEvent, sizeof(keyboard_event_t), &olen, (const uint8_t*)base64_cstr, doc["event"].get_ref<nlohmann::json::string_t&>().size());
    if (res != 0) {
        return;
    }
    recvOverlayKeyboardEventDelegate(doc["windowId"].get_ref<nlohmann::json::number_integer_t&>(), outEvent);
}
