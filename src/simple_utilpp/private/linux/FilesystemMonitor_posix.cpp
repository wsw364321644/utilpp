#include "FilesystemMonitor.h"
#include "simple_os_defs.h"
#include "std_ext.h"
#include "dir_util_internal.h"
#include "string_convert.h"
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

#include <memory>
#include <string>
#include <filesystem>
#include <unorder_map>
#include <assert.h>

#define MAX_EVENTS       4096
/**
 * MAX_EPOLL_EVENTS is set to 1 since there exists
 * only one eventbuffer. The value can be increased
 * when readEventsIntoBuffer can handle multiple
 * epoll events.
 */
#define MAX_EPOLL_EVENTS 1
#define EVENT_SIZE       (sizeof (inotify_event))

typedef struct PathMonitorData_t
{
    ~PathMonitorData_t() {
    }
    CommonHandle_t Handle;
    IFilesystemMonitor::TMonitorCallback Delegate;
    std::string Path;
    uint32_t Mask;
    uint32_t InotifyMask;
    std::vector<uint8_t> ResultBuf;
    std::unorder_map<std::string_view, int> WatchsPathMap;
    std::unorder_map<int,std::string> WatchsIDMap;
} PathMonitorData_t;

class FFilesystemMonitorPosix : public IFilesystemMonitor
{
public:
    FFilesystemMonitorPosix()
    :EventBuffer(MAX_EVENTS * (sizeof (inotify_event) + 16), 0)
     {
        std::stringstream errorStream;

        InotifyFd = inotify_init();
        if (InotifyFd < 0) {
            errorStream << "Can't initialize inotify ! " << strerror(errno) << ".";
            throw std::runtime_error(errorStream.str());
        }

        //if (pipe(StopPipeFd, O_NONBLOCK) == -1) {
        //    errorStream << "Can't initialize stop pipe ! " << strerror(errno) << ".";
        //    throw std::runtime_error(errorStream.str());
        //}
        //int flags = fcntl(StopPipeFd, F_GETFL, 0);
        //fcntl(StopPipeFd, F_SETFL, flags | O_NONBLOCK);

        EpollFd = epoll_create1(0);
        if (EpollFd  < 0) {
            errorStream << "Can't initialize epoll ! " << strerror(errno) << ".";
            throw std::runtime_error(errorStream.str());
        }

        InotifyEpollEvent.events = EPOLLIN | EPOLLET;
        InotifyEpollEvent.data.fd = InotifyFd;
        if (epoll_ctl(EpollFd, EPOLL_CTL_ADD, InotifyFd, &InotifyEpollEvent) == -1) {
            errorStream << "Can't add inotify filedescriptor to epoll ! " << strerror(errno) << ".";
            throw std::runtime_error(errorStream.str());
        }
    }
    ~FFilesystemMonitorPosix() {
        std::stringstream errorStream;
        epoll_ctl(EpollFd, EPOLL_CTL_DEL, InotifyFd, 0);
    
        if (!close(InotifyFd)) {
            errorStream << "close failed " << strerror(errno) << ".";
            throw std::runtime_error(errorStream.str());
        }
    
        if (!close(EpollFd)) {
            errorStream << "close failed " << strerror(errno) << ".";
            throw std::runtime_error(errorStream.str());
        }
    }
    CommonHandle_t Monitor(std::u8string_view path, uint32_t mask, TMonitorCallback) override;
    void CancelMonitor(CommonHandle_t handle) override;
    void Tick(float delta)override;
private:
    bool MonitorAddOnePath(PathMonitorData_t& PathMonitorData,std::filesystem::path& path);
    void RemoveWatchs(PathMonitorData_t& PathMonitorData);
    void RemoveWatch(int wd);
    FPathBuf PathBuf;
    int InotifyFd;

    int EpollFd;
    epoll_event InotifyEpollEvent;
    epoll_event EpollEvents[MAX_EPOLL_EVENTS];
    std::vector<uint8_t> EventBuffer;

    std::unordered_map< CommonHandle_t, std::shared_ptr<PathMonitorData_t>> MonitorDatas;
    std::unordered_map< std::u8string_view, CommonHandle_t, string_hash> MonitorDataPathMap;
    std::unordered_map< int,CommonHandle_t> WDCheckMap;
    std::unordered_map< uint32_t ,std::filesystem::path> RenameOldMap;
    std::unordered_map< uint32_t ,std::filesystem::path> RenameNewMap;
};
CommonHandle_t FFilesystemMonitorPosix::Monitor(std::u8string_view pathstr, uint32_t mask, TMonitorCallback CB)
{
    if (MonitorDataPathMap.find(pathstr) != MonitorDataPathMap.end()) {
        return NullHandle;
    }
    std::filesystem::path path(pathstr);
    std::error_code ec;
    if (!std::filesystem::exists(pathstr, ec) || ec
        || !std::filesystem::is_directory(path, ec) || ec) {
        return NullHandle;
    }

    auto pPathMonitorData = std::make_shared<PathMonitorData_t>();
    auto& PathMonitorData = *pPathMonitorData;
    PathMonitorData.Path.assign((const char*)pathstr.data(), (const char*)pathstr.data() + pathstr.length());
    PathMonitorData.Mask = mask;
    PathMonitorData.Delegate = CB;

    if (mask & MONITOR_MASK_ACCESS) PathMonitorData.InotifyMask |= IN_ACCESS;
    if (mask & MONITOR_MASK_OPEN) PathMonitorData.InotifyMask |= IN_OPEN| IN_CLOSE_WRITE| IN_CLOSE_NOWRITE;
    if (mask & MONITOR_MASK_ATTRIB) PathMonitorData.InotifyMask |= IN_ATTRIB;
    if (mask & MONITOR_MASK_NAME_CHANGE) PathMonitorData.InotifyMask |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
    if (mask & MONITOR_MASK_CREATE) PathMonitorData.InotifyMask |= IN_CREATE;
    if (mask & MONITOR_MASK_DELETE) PathMonitorData.InotifyMask |= IN_DELETE| IN_DELETE_SELF;
    if (mask & MONITOR_MASK_RENAME) PathMonitorData.InotifyMask |= IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO;
    if (mask & MONITOR_MASK_MODIFY) PathMonitorData.InotifyMask |= IN_MODIFY;

    auto [itr, res] = MonitorDatas.try_emplace(CommonHandle_t(CommonHandle_t::atomic_count), pPathMonitorData);
    PathMonitorData.Handle = itr->first;
    MonitorDataPathMap.try_emplace(ConvertStringTotU8View(PathMonitorData.Path), PathMonitorData.Handle);

    MonitorAddOnePath(PathMonitorData, pathstr);
    std::ranges::for_each(
        std::filesystem::recursive_directory_iterator{ pathstr },
        [](const std::filesystem::directory_entry& dir_entry) {
            if (!dir_entry.is_directory()) {
                return;
            }
            MonitorAddOnePath(PathMonitorData, dir_entry.path());
        }
    );

    return PathMonitorData.Handle;
}
void FFilesystemMonitorPosix::CancelMonitor(CommonHandle_t handle)
{
    auto itr = MonitorDatas.find(handle);
    if (itr == MonitorDatas.end()) {
        return;
    }
    auto& [key, data] = *itr;
    RemoveWatchs(*data);
    MonitorDataPathMap.erase(ConvertStringTotU8View(data->Path));
    MonitorDatas.erase(itr);
}

void FFilesystemMonitorPosix::Tick(float delta)
{
    ssize_t length = 0;
    auto timeout = 0;
    auto nFdsReady = epoll_wait(EpollFd, EpollEvents, MAX_EPOLL_EVENTS, timeout);

    if (nFdsReady == -1) {
        return;
    }

    for (auto n = 0; n < nFdsReady; ++n) {
        length = read(EpollEvents[n].data.fd, EventBuffer.data(), EventBuffer.size());
        if (length == -1) {
            if(errno == EINTR){
                return;
            }
        }
    }
    int i = 0;
    while (i < length) {
        inotify_event* event = (struct inotify_event*)(EventBuffer.data()+i);
        auto itr=WDCheckMap.find(event->wd);
        if(itr == WDCheckMap.end()){
            i += sizeof (inotify_event) + event->len;
            continue;
        }
        auto& [wd, handle] = *itr;
        PathMonitorData_t& PathMonitorData=*MonitorDatas[itr->second];
        auto& wdPath = PathMonitorData.WatchsIDMap[event->wd];
        std::filesystem::path path(wdPath);
        path /= std::string_view(event->name,strlen(event->name));

        if(event->mask & IN_IGNORED){
            i += sizeof (inotify_event) + event->len;
            PathMonitorData.WatchsPathMap.erase(PathMonitorData.WatchsIDMap[event->wd]);
            PathMonitorData.WatchsIDMap.erase(event->wd);
            WDCheckMap.erase(itr);
            continue;
        }
        if(event->mask & IN_DELETE){
            PathMonitorData.Delegate(EFilesystemAction::FSA_DELETE,path.u8string(), std::u8string_view());
        }
        if(event->mask & IN_CREATE ){
            PathMonitorData.Delegate(EFilesystemAction::FSA_CREATE,path.u8string(), std::u8string_view());
        }

        if(event->mask & IN_OPEN ){
            PathMonitorData.Delegate(EFilesystemAction::FSA_OPEN,path.u8string(), std::u8string_view());
        }
        if(event->mask & IN_ACCESS ){
            PathMonitorData.Delegate(EFilesystemAction::FSA_ACCESS,path.u8string(), std::u8string_view());
        }
        if(event->mask & IN_MODIFY){
            PathMonitorData.Delegate(EFilesystemAction::FSA_MODIFY,path.u8string(),std::u8string_view() );
        }
        if(event->mask & IN_ATTRIB){
            PathMonitorData.Delegate(EFilesystemAction::FSA_ATTRIB,path.u8string(), std::u8string_view());
        }
        if(event->mask & IN_MOVED_FROM){
            auto itr=RenameNewMap.find(event->cookie);
            if(itr==RenameNewMap.end()){
                RenameOldMap.try_emplace(event->cookie, path);
            }else{
                PathMonitorData.Delegate(EFilesystemAction::FSA_RENAME, path.u8string(),itr->second.u8string());
                RenameNewMap.erase(itr);
            }
        }
        if(event->mask & IN_MOVED_TO){
            auto itr=RenameOldMap.find(event->cookie);
            if(itr==RenameOldMap.end()){
                RenameNewMap.try_emplace(event->cookie, path);
            }else{
                PathMonitorData.Delegate(EFilesystemAction::FSA_RENAME, itr->second.u8string(),path.u8string());
                RenameOldMap.erase(itr);
            }
        }
        i += sizeof (inotify_event) + event->len;
    }
}

bool MonitorAddOnePath(PathMonitorData_t& PathMonitorData, std::filesystem::path& path)
{
    auto const id = inotify_add_watch(InotifyFd, path.c_str(), PathMonitorData.InotifyMask);
    auto [itr, res] = PathMonitorData.WatchsIDMap.try_emplace(id,path.string());
    PathMonitorData.WatchsIDMap.try_emplace(itr->second, id);
    WDCheckMap.try_emplace(id, PathMonitorData.Handle);
}

void RemoveWatchs(PathMonitorData_t& PathMonitorData){
    for(auto& [ wd,path] : PathMonitorData.WatchsIDMap) {
        RemoveWatch(wd)
        WDCheckMap.erase(wd);
    }
}

void RemoveWatch(int wd){
    auto result = inotify_rm_watch(InotifyFd, wd);
    if (result == -1) {
        std::stringstream errorStream;
        errorStream << "Failed to remove watch! " << strerror(errno) << ".";
        throw std::runtime_error(errorStream.str());
    }
}
IFilesystemMonitor* GetFilesystemMonitor()
{
    static std::atomic<std::shared_ptr<FFilesystemMonitorPosix>> atomicptr;
    auto oldptr = atomicptr.load();
    if (!oldptr) {
        std::shared_ptr<FFilesystemMonitorPosix> ptr(new FFilesystemMonitorPosix);
        atomicptr.compare_exchange_strong(oldptr, ptr);
    }
    return atomicptr.load().get();
}