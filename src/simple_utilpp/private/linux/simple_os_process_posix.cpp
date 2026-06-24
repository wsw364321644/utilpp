#include "simple_os_process.h"
#include <sys/types.h>
pid_t get_pid()
{
    return ::getpid();
}

bool kill_process(pid_t pid, int exitCode)
{
    return ::kill(pid, exitCode) == 0;
}

bool isProcessUsingPipe(const char *pid, const char *pipePath)
{
    std::string fdDir = std::string("/proc/") + pid + "/fd";
    DIR *dir = opendir(fdDir.c_str());
    if (!dir)
        return false;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_type == DT_LNK)
        {
            char linkPath[PATH_MAX];
            snprintf(linkPath, sizeof(linkPath), "%s/%s", fdDir.c_str(), entry->d_name);

            char target[PATH_MAX];
            ssize_t len = readlink(linkPath, target, sizeof(target) - 1);
            if (len != -1)
            {
                target[len] = '\0';
                if (std::string(target) == pipePath)
                {
                    closedir(dir);
                    return true;
                }
            }
        }
    }
    closedir(dir);
    return false;
}

bool get_pipe_client_proc_id_from_pipe_name(const char *handle, pid_t *id)
{
    DIR *procDir = opendir("/proc");
    if (!procDir)
    {
        std::cerr << "Failed to open /proc directory." << std::endl;
        return false;
    }

    struct dirent *entry;
    while ((entry = readdir(procDir)) != nullptr)
    {
        // Check if the entry is a directory and represents a process ID
        if (entry->d_type == DT_DIR && std::all_of(entry->d_name, entry->d_name + strlen(entry->d_name), ::isdigit))
        {
            if (isProcessUsingPipe(entry->d_name, (const char *)handle))
            {
                *id = std::stoi(entry->d_name);
                break;
            }
        }
    }

    closedir(procDir);
    return true;
}
bool get_pipe_client_proc_id(F_HANDLE handle, pid_t *id)
{
    return false;
}
void iterate_process(FProcessInfoFunc func)
{
    popen("ps -A -o pid=", "r");
}

#define MAXBUF (BUFSIZ * 2)
pid_t get_proc_parent_id(pid_t id)
{
    if (id == std::numeric_limits<uint64_t>::max())
    {
        return getppid();
    }
    pid_t ppid = std::numeric_limits<uint64_t>::max();
    char buf[MAXBUF];
    char procname[32]; // Holds /proc/4294967296/status\0
    FILE *fp;

    snprintf(procname, sizeof(procname), "/proc/%u/status", pid);
    fp = fopen(procname, "r");
    if (fp != NULL)
    {
        size_t ret = fread(buf, sizeof(char), MAXBUF - 1, fp);
        if (!ret)
        {
            return ppid;
        }
        else
        {
            buf[ret++] = '\0'; // Terminate it.
        }
    }
    fclose(fp);
    char *ppid_loc = strstr(buf, "\nPPid:");
    if (!ppid_loc)
    {
        return ppid;
    }
    int ret = sscanf(ppid_loc, "\nPPid:%d", &ppid);
    if (!ret || ret == EOF)
    {
        return ppid;
    }
    return ppid;
}

std::vector<save_memory_operator_string, allocator_save_memory_operator<save_memory_operator_string>> get_command_line(std::error_code &ec)
{
    std::vector<save_memory_operator_string, allocator_save_memory_operator<save_memory_operator_string>> out;
    int argc;
    ec.clear();
    char sz[1024 + 1];
    int dw = readlink("/proc/self/cmdline", sz, 1024);
    if (dw < 1)
    {
        ec = std::error_code(errno, std::system_category());
        return out;
    }
    sz[dw] = '\x0';
    int index = 0;
    for (int i = 0; i < dw; i++)
    {
        if (sz[i] == '\0')
        {
            out.push_back(save_memory_operator_string(sz + index, sz + i));
        }
    }
    return out;
}


typedef struct ProcessInfoWin_t {
    HANDLE HProcess;
}ProcessInfoWin_t;
CommonHandlePtr_t open_process(pid_t pid)
{
    // POSIX 标准没有 OpenProcess，通常通过 kill(pid, 0) 来探测进程是否存在
    // 注意：如果进程存在但属于其他用户且非 root，kill 可能会返回 EPERM
    if (kill(pid, 0) != 0) {
        return NullHandle; 
    }

    auto ptr = new ProcessInfoPosix_t;
    ptr->pid = pid;
    ptr->is_reaped = false;
    
    return CommonHandlePtr_t(intptr_t(ptr));
}

void close_process_handle(CommonHandlePtr_t handle)
{
    if (!handle) {
        return;
    }
    auto ptr = (ProcessInfoWin_t*)handle.ID;
    CloseHandle(ptr->HProcess);
    delete ptr;
    handle.Reset();
}

bool is_process_exist(CommonHandlePtr_t handle)
{
   if (!handle) {
        return false;
    }
    auto ptr = (ProcessInfoPosix_t*)handle.ID;

    // 如果之前已经确认进程退出并被回收，直接返回 false
    if (ptr->is_reaped) {
        return false;
    }

    // 使用 WNOHANG 进行非阻塞探测，等效于 Windows 的 WaitForSingleObject(h, 0)
    int status = 0;
    pid_t result = waitpid(ptr->pid, &status, WNOHANG);

    if (result == 0) {
        // 进程仍在运行（等效于 WAIT_TIMEOUT）
        return true;
    } 
    else if (result > 0) {
        // 进程已退出并被系统回收（等效于 WAIT_OBJECT_0）
        ptr->is_reaped = true;
        return false;
    } 
    else {
        // waitpid 返回 -1，发生错误
        if (errno == ECHILD) {
            // 该进程不是当前进程的子进程，或者已经被回收
            // 此时无法通过 waitpid 判断，退化为使用 kill(pid, 0) 探测
            if (kill(ptr->pid, 0) == 0) {
                return true; // 进程还在，只是不归我们管
            } else {
                ptr->is_reaped = true;
                return false; // 进程已退出
            }
        } else {
            // 其他错误（如 EINTR），保守返回 true 或打印日志
            // assert("is_process_exist waitpid error: %s\n", strerror(errno));
            return true; 
        }
    }
}
