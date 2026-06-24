#include "simple_os_process.h"
#include <FunctionExitHelper.h>
#include <string_convert.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <assert.h>
pid_t get_pid() {
    return ::GetCurrentProcessId();
}

bool kill_process(pid_t pid,int exitCode)
{
    auto handle = OpenProcess(PROCESS_TERMINATE, false, pid);
    if (handle == NULL) {
        return false;
    }
    return ::TerminateProcess(handle, exitCode)!=0;
}


bool get_pipe_client_proc_id_from_pipe_name(const char* name, pid_t* id)
{
    //static_assert(false, "Windows can't create pipe without known it's properties");
    return false;
}
bool get_pipe_client_proc_id(F_HANDLE handle, pid_t* id)
{
    ULONG out;
    if (::GetNamedPipeClientProcessId(handle, &out)) {
        *id = out;
        return true;
    }
    return false;
}

void iterate_process(FProcessInfoFunc func) {
    HANDLE handle;
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    ProcessInfo_t out;
    handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }
    if (Process32First(handle, &pe)) {
        do {
            out.Pid = pe.th32ProcessID;
            out.PathStr = pe.szExeFile;
            func(out);
        } while (Process32Next(handle, &pe));
    }
    CloseHandle(handle);
    return;
}

pid_t get_proc_parent_id(pid_t id)
{
    pid_t parent_pid = std::numeric_limits<uint64_t>::max();
    HANDLE handle;
    PROCESSENTRY32 pe;
    DWORD current_pid = GetCurrentProcessId();

    if (id == std::numeric_limits<uint64_t>::max()) {
        current_pid = GetCurrentProcessId();
    }
    else {
        current_pid = id;
    }

    pe.dwSize = sizeof(PROCESSENTRY32);
    handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(handle, &pe)) {
        do {
            if (pe.th32ProcessID == current_pid) {
                parent_pid = pe.th32ParentProcessID;
                break;
            }
        } while (Process32Next(handle, &pe));
    }

    CloseHandle(handle);
    return parent_pid;
}

std::vector<save_memory_operator_string, allocator_save_memory_operator<save_memory_operator_string>> get_command_line(std::error_code& ec)
{
    std::vector<save_memory_operator_string, allocator_save_memory_operator<save_memory_operator_string>> out;
    int argc;
    ec.clear();
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argvW == nullptr) {
        return out;
    }
    FunctionExitHelper_t argvWExitHelper(
        [argvW]() {
            LocalFree(argvW);
        }
    );

    for (int i = 0; i < argc; ++i) {
        out.push_back(U16ToU8<save_memory_operator_string>(argvW[i], GetStringLengthW(argvW[i])));
    }
    return out;
}

typedef struct ProcessInfoWin_t {
    HANDLE hProcess;
}ProcessInfoWin_t;
CommonHandlePtr_t open_process(pid_t pid)
{
    auto handle = OpenProcess(SYNCHRONIZE, false, pid);
    if (handle == NULL) {
        return NullHandle;
    }
    auto ptr = new ProcessInfoWin_t;
    ptr->hProcess = handle;
    return CommonHandlePtr_t(intptr_t(ptr));
}

void close_process_handle(CommonHandlePtr_t handle)
{
    if (!handle) {
        return;
    }
    auto ptr = (ProcessInfoWin_t*)handle.ID;
    CloseHandle(ptr->hProcess);
    delete ptr;
    handle.Reset();
}

bool is_process_exist(CommonHandlePtr_t handle)
{
    if (!handle) {
        return false;
    }
    auto ptr = (ProcessInfoWin_t*)handle.ID;
    DWORD result = WaitForSingleObject(ptr->hProcess, 0);
    if (result == WAIT_OBJECT_0) {
        return false;
    }
    else if (result == WAIT_TIMEOUT) {
        return true;
    }
    else {
        GetLastError();
        assert("is_process_exist WaitForSingleObject error");
        return false;
    }
}
