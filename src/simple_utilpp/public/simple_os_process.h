#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <functional>
#include "simple_os_defs.h"
#include "simple_export_defs.h"
#include "simple_export_ppdefs.h"


SIMPLE_UTIL_API pid_t get_pid();
SIMPLE_UTIL_API bool kill_process(pid_t, int);
SIMPLE_UTIL_API bool get_pipe_client_proc_id_from_pipe_name(const char* name, pid_t* id);
SIMPLE_UTIL_API bool get_pipe_client_proc_id(F_HANDLE handle, pid_t* id);
typedef struct ProcessInfo_t
{
    pid_t Pid;
    const char* PathStr;
}ProcessInfo_t;
typedef std::function<void(ProcessInfo_t&)> FProcessInfoFunc;
SIMPLE_UTIL_EXPORT void iterate_process(FProcessInfoFunc);

SIMPLE_UTIL_EXPORT pid_t get_proc_parent_id(pid_t id = std::numeric_limits<uint64_t>::max());