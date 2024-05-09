#pragma once

#include <RPC/rpc_processer.h>
#include <RPC/jrpc_interface.h>
#include "RpcHookHelperEventInterface.h"
#include "HOOK/keyboard_event.h"
#include "HOOK/mouse_event.h"
#include "HOOK/hotkey_list.h"
#pragma warning(push)
#pragma warning(disable:4251)

typedef std::map<std::string, std::vector<key_with_modifier_t>> HotKeyList_t;

class HOOK_HELPER_EXPORT JRPCHookHelperEventAPI :public IGroupJRPC, public IRPCHookHelperEventAPI
{
public:
    DECLARE_RPC_OVERRIDE_FUNCTION(JRPCHookHelperEventAPI);

    
    DECLARE_REQUEST_RPC_EVENT_ONE_PARAM(HotkeyListUpdate, HotKeyList_t&);
    DECLARE_REQUEST_RPC_EVENT_TWO_PARAM(OverlayMouseWheelEvent, uint64_t, mouse_wheel_event_t&);
    DECLARE_REQUEST_RPC_EVENT_TWO_PARAM(OverlayMouseButtonEvent, uint64_t, mouse_button_event_t&);
    DECLARE_REQUEST_RPC_EVENT_TWO_PARAM(OverlayMouseMotionEvent, uint64_t, mouse_motion_event_t&);
    DECLARE_REQUEST_RPC_EVENT_TWO_PARAM(OverlayKeyboardEvent, uint64_t, keyboard_event_t&);

};
#pragma warning(pop)
