#include "event_util.h"
#include "ipc_error_internal.h"
#include <char_buffer_extension.h>
#include <simple_os_defs.h>
#include <unordered_map>
#include <filesystem>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <errno.h>

typedef struct EventInfoPosix_t {
    sem_t * Mutex{ NULL };
    bool Owner{ false };
}EventInfoPosix_t;

namespace utilpp {
    const CommonHandlePtr_t CreateProcEvent(std::string_view name, std::error_code& ec) {
        EventInfoPosix_t info;
        info.Mutex=sem_open(name.data(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
        if(info.Mutex==SEM_FAILED){
            if(errno==EEXIST){
                ec = make_ipc_error(EIPCError::IPCE_AlreadyExist);
            }
            else{
                ec = make_ipc_error(EIPCError::IPCE_Unknow);
            }
            return NullHandle;
        }
        info.Owner=true;
        ec = make_ipc_error(EIPCError::IPCE_OK);

        CommonHandlePtr_t HPtr((intptr_t)new EventInfoPosix_t(info));
        return HPtr;
    }

    const CommonHandlePtr_t OpenProcEvent(std::string_view name, std::error_code& ec)
    {
        EventInfoPosix_t info;
        info.Mutex=sem_open(name.data(), NULL , S_IRUSR | S_IWUSR, 1);
        if(info.Mutex==SEM_FAILED){
            if(errno==ENOENT ){
                ec = make_ipc_error(EIPCError::IPCE_NotExist);
            }
            else{
                ec = make_ipc_error(EIPCError::IPCE_Unknow);
            }
            return NullHandle;
        }
        CommonHandlePtr_t HPtr((intptr_t)new EventInfoPosix_t(info);
        return HPtr;
    }

    bool SetProcEvent(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        EventInfoPosix_t& info = *(EventInfoPosix_t*)handle.ID;
        return sem_post(info.Mutex)==0;
    }

    bool PeekProcEvent(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        EventInfoPosix_t& info = *(EventInfoPosix_t*)handle.ID;
        auto ires = sem_trywait(info.Mutex);
        if (ires != 0) {
            ec = make_ipc_error(EIPCError::IPCE_Unknow);
            return false;
        }
        return true;
    }

    bool WaitForProcEventInfinite(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        EventInfoPosix_t& info = *(EventInfoPosix_t*)handle.ID;
        auto ires = sem_wait(info.Mutex);
        if (ires != 0) {
            ec = make_ipc_error(EIPCError::IPCE_Unknow);
            return false;
        }
        return true;
    }

    bool WaitForProcEvent(const CommonHandlePtr_t handle, uint64_t microSec, std::error_code& ec)
    {
        EventInfoPosix_t& info = *(EventInfoPosix_t*)handle.ID;
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            return false;
        }
        ts.tv_sec+=microSec/1000000;
        ts.tv_nsec+= (microSec-microSec/1000000)*1000;
        auto ires = sem_timedwait(info.Mutex,&ts);
        if (ires != 0) {
            if(errno==ETIMEDOUT){
                ec = make_ipc_error(EIPCError::IPCE_Timeout);
            }
            else{
                ec = make_ipc_error(EIPCError::IPCE_Unknow);
            }
            return false;
        }
        return true;
    }

    bool CloseProcEvent(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        f (!handle) {
            ec = make_ipc_error(EIPCError::IPCE_NotExist);
            return false;
        }
        EventInfoPosix_t& info = *(EventInfoPosix_t*)handle.ID;
        auto ires=sem_close(info.Mutex);
        if(ires!=0){
            ec = make_ipc_error(EIPCError::IPCE_Unknow);
            return false;
        }
        if(info.Owner){
            sem_unlink(info.Mutex);
        }
        delete (EventInfoPosix_t*)handle.ID;
        const_cast<CommonHandlePtr_t&>(handle).Reset();
        return true;
    }

}