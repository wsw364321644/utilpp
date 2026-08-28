#include "event_util.h"
#include "ipc_error_internal.h"
#include <char_buffer_extension.h>
#include <simple_os_defs.h>
#include <unordered_map>
#include <filesystem>
#include <AclAPI.h>
typedef struct EventInfoWin_t {
    HANDLE Handle{ NULL };
}EventInfoWin_t;

namespace utilpp {
    const CommonHandlePtr_t CreateProcEvent(std::string_view name, std::error_code& ec) {
        FCharBuffer buf;
        EventInfoWin_t info;
        const char* nameStr = GetStringViewCStr(name, buf);

        // 1. 初始化 Everyone SID
        PSID pEveryoneSid = NULL;
        SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
        AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0, &pEveryoneSid);

        // 2. 设置 Everyone 拥有 EVENT_MODIFY_STATE | SYNCHRONIZE 权限
        EXPLICIT_ACCESS_A ea = { 0 };
        ea.grfAccessPermissions = EVENT_MODIFY_STATE | SYNCHRONIZE;
        ea.grfAccessMode = SET_ACCESS;
        ea.grfInheritance = NO_INHERITANCE;
        ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea.Trustee.ptstrName = (LPTSTR)pEveryoneSid;

        // 3. 创建 ACL 和安全描述符
        PACL pAcl = NULL;
        SetEntriesInAclA(1, &ea, NULL, &pAcl);

        SECURITY_DESCRIPTOR sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE);

        // 4. 用这个安全属性创建事件
        SECURITY_ATTRIBUTES sa = { sizeof(sa), &sd, FALSE };

        info.Handle = CreateEventA(&sa, false, false, nameStr);
        DWORD dwRet = GetLastError();
        if (!info.Handle) {
            if (ERROR_ACCESS_DENIED == dwRet) {
                ec = make_ipc_error(EIPCError::IPCE_AccessDenied);
            }
            else {
                ec = make_ipc_error(EIPCError::IPCE_Unknow);
            }
            return NullHandle;
        }

        if (ERROR_ALREADY_EXISTS == dwRet) {
            ec = make_ipc_error(EIPCError::IPCE_AlreadyExist);
        }
        else {
            ec = make_ipc_error(EIPCError::IPCE_OK);
        }
        CommonHandlePtr_t HPtr((intptr_t)new EventInfoWin_t(info));
        return HPtr;
    }

    const CommonHandlePtr_t OpenProcEvent(std::string_view name, std::error_code& ec)
    {
        FCharBuffer buf;
        EventInfoWin_t info;
        const char* nameStr = GetStringViewCStr(name, buf);
        info.Handle = OpenEventA((EVENT_MODIFY_STATE | SYNCHRONIZE), false, nameStr);
        if (!info.Handle) {
            DWORD dwRet = GetLastError();
            switch (dwRet) {
            case ERROR_ACCESS_DENIED: {
                ec = make_ipc_error(EIPCError::IPCE_AccessDenied);
                break;
            }
            default: {
                ec = make_ipc_error(EIPCError::IPCE_NotExist);
                
            }
            }
            return NullHandle;
        }
        ec = make_ipc_error(EIPCError::IPCE_OK);
        CommonHandlePtr_t HPtr((intptr_t)new EventInfoWin_t(info));
        return HPtr;
    }

    bool SetProcEvent(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        EventInfoWin_t& info = *(EventInfoWin_t*)handle.ID;
        return SetEvent(info.Handle);
    }

    bool PeekProcEvent(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        return WaitForProcEvent(handle, 0, ec);
    }

    bool WaitForProcEventInfinite(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        return WaitForProcEvent(handle, std::numeric_limits<uint64_t>::max(), ec);
    }

    bool WaitForProcEvent(const CommonHandlePtr_t handle, uint64_t microSec, std::error_code& ec)
    {
        EventInfoWin_t& info = *(EventInfoWin_t*)handle.ID;
        DWORD time = DWORD(microSec / 1000);
        auto dwWaitResult = WaitForSingleObject(
            info.Handle,
            time);
        if (dwWaitResult != WAIT_OBJECT_0) {
            if (dwWaitResult == WAIT_TIMEOUT) {
                ec = make_ipc_error(EIPCError::IPCE_Timeout);
            }else{
                ec = make_ipc_error(EIPCError::IPCE_Unknow);
            }
            return false;
        }
        return true;
    }

    bool CloseProcEvent(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        if (!handle) {
            ec = make_ipc_error(EIPCError::IPCE_NotExist);
            return false;
        }
        EventInfoWin_t& info = *(EventInfoWin_t*)handle.ID;
        auto bres = CloseHandle(info.Handle);
        if (bres) {
            delete (EventInfoWin_t*)handle.ID;
            const_cast<CommonHandlePtr_t&>(handle).Reset();
        }
        return bres;
    }

}