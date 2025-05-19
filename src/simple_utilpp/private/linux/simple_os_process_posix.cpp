#include "simple_os_process.h"
#include <sys/types.h>
pid_t get_pid()
{
    return ::getpid();
}

bool kill_process(pid_t pid, int exitCode)
{
    return ::kill(pid, exitCode)==0;
}


bool isProcessUsingPipe(const char* pid, const char* pipePath) {
    std::string fdDir = std::string("/proc/") + pid + "/fd";
    DIR* dir = opendir(fdDir.c_str());
    if (!dir) return false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_LNK) {
            char linkPath[PATH_MAX];
            snprintf(linkPath, sizeof(linkPath), "%s/%s", fdDir.c_str(), entry->d_name);

            char target[PATH_MAX];
            ssize_t len = readlink(linkPath, target, sizeof(target) - 1);
            if (len != -1) {
                target[len] = '\0';
                if (std::string(target) == pipePath) {
                    closedir(dir);
                    return true;
                }
            }
        }
    }
    closedir(dir);
    return false;
}

bool get_pipe_client_proc_id_from_pipe_name(const char*  handle, pid_t* id)
{
    DIR* procDir = opendir("/proc");
    if (!procDir) {
        std::cerr << "Failed to open /proc directory." << std::endl;
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        // Check if the entry is a directory and represents a process ID
        if (entry->d_type == DT_DIR && std::all_of(entry->d_name, entry->d_name + strlen(entry->d_name), ::isdigit)) {
            if (isProcessUsingPipe(entry->d_name, (const char*)handle)) {
                *id = std::stoi(entry->d_name);
                break;
            }
        }
    }

    closedir(procDir);
    return true;
}
bool get_pipe_client_proc_id(F_HANDLE handle, pid_t* id) {
    return false;
}
void iterate_process(FProcessInfoFunc func) {
    popen("ps -A -o pid=", "r");
}

#define MAXBUF      (BUFSIZ * 2)
pid_t get_proc_parent_id(pid_t id){
    if (id == std::numeric_limits<uint64_t>::max()) {
        return getppid();
    }
    pid_t ppid= std::numeric_limits<uint64_t>::max();
    char buf[MAXBUF];
    char procname[32];  // Holds /proc/4294967296/status\0
    FILE* fp;

    snprintf(procname, sizeof(procname), "/proc/%u/status", pid);
    fp = fopen(procname, "r");
    if (fp != NULL) {
        size_t ret = fread(buf, sizeof(char), MAXBUF - 1, fp);
        if (!ret) {
            return ppid;
        }
        else {
            buf[ret++] = '\0';  // Terminate it.
        }
    }
    fclose(fp);
    char* ppid_loc = strstr(buf, "\nPPid:");
    if (!ppid_loc) {
        return ppid;
    }
    int ret = sscanf(ppid_loc, "\nPPid:%d", &ppid);
    if (!ret || ret == EOF) {
        return ppid;
    }
    return ppid;
}

