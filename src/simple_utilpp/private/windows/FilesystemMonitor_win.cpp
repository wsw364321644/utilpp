#include "FilesystemMonitor.h"
#include "simple_os_defs.h"
#include "std_ext.h"
#include "dir_util_internal.h"
#include "string_convert.h"
#include <memory>
#include <string>
#include <filesystem>
#include <assert.h>

constexpr int FILE_NOTIFY_INFO_CACHE_SIZE = 100;
typedef struct PathMonitorData_t
{
    ~PathMonitorData_t(){
        if (DirHandle != INVALID_HANDLE_VALUE) {
            if (OverlappedData.hEvent != INVALID_HANDLE_VALUE) {
                CloseHandle(OverlappedData.hEvent);
                CancelIoEx(DirHandle, &OverlappedData);
            }
            CloseHandle(DirHandle);
        }

    }
    IFilesystemMonitor::TMonitorCallback Delegate;
    CommonHandle_t Handle;
    std::string Path;
    uint32_t Mask;
    HANDLE DirHandle{ INVALID_HANDLE_VALUE };
    
    OVERLAPPED OverlappedData{.hEvent= INVALID_HANDLE_VALUE };
    std::vector<uint8_t> ResultBuf;
    DWORD NotifyFilter{ 0 };
} PathMonitorData_t;

class FFilesystemMonitorWin : public IFilesystemMonitor
{
public:
    FFilesystemMonitorWin() {
        CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
        if (CompletionPort == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("CreateIoCompletionPort failed");
        }
    }
    ~FFilesystemMonitorWin() {
        MonitorDatas.clear();
        MonitorDataPathMap.clear();
        if (CompletionPort != INVALID_HANDLE_VALUE) {
            CloseHandle(CompletionPort);
        }
    }
    CommonHandle_t Monitor(std::u8string_view path, uint32_t mask, TMonitorCallback) override;
    void CancelMonitor(CommonHandle_t handle) override;
    void Tick(float delta)override;
private:
    bool InternalStartMonitor(PathMonitorData_t& PathMonitorData);
    FPathBuf PathBuf;
    HANDLE CompletionPort{ INVALID_HANDLE_VALUE };
    std::unordered_map< CommonHandle_t, std::shared_ptr<PathMonitorData_t>> MonitorDatas;
    std::unordered_map< std::u8string_view, CommonHandle_t, string_hash> MonitorDataPathMap;
};
CommonHandle_t FFilesystemMonitorWin::Monitor(std::u8string_view pathstr, uint32_t mask, TMonitorCallback CB)
{
    if (MonitorDataPathMap.find(pathstr) != MonitorDataPathMap.end()) {
        return NullHandle;
    }
    std::filesystem::path path(pathstr);
    std::error_code ec;
    if (!std::filesystem::exists(pathstr,ec)||ec
        || !std::filesystem::is_directory(path, ec)||ec) {
        return NullHandle;
    }
    
    PathBuf.SetPath((char*)pathstr.data(), pathstr.length());
    PathBuf.ToPathW();
    auto pathw = PathBuf.GetPrependFileNamespacesW();
    auto DirHandle = CreateFileW(
        pathw,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    if (DirHandle == INVALID_HANDLE_VALUE) {
        return NullHandle;
    }
    std::string NotifyEventName = "FileChangedEvent";
    NotifyEventName.append( std::to_string(intptr_t(DirHandle)));
    auto EventHandle = CreateEventA(NULL, FALSE, FALSE, NotifyEventName.c_str());
    if (EventHandle == INVALID_HANDLE_VALUE) {
        CloseHandle(DirHandle);
        return NullHandle;
    }
    auto pPathMonitorData = std::make_shared<PathMonitorData_t>();
    auto& PathMonitorData = *pPathMonitorData;
    PathMonitorData.Path.assign((const char*)pathstr.data(), (const char*)pathstr.data() + pathstr.length());
    PathMonitorData.DirHandle = DirHandle;
    PathMonitorData.OverlappedData.hEvent = EventHandle;
    PathMonitorData.Mask = mask;
    PathMonitorData.ResultBuf.resize(1024*4);
    PathMonitorData.Delegate = CB;
    // Set up the filter for the notifications
    if (mask & MONITOR_MASK_ACCESS) PathMonitorData.NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
    if (mask & MONITOR_MASK_OPEN) PathMonitorData.NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
    if (mask & MONITOR_MASK_ATTRIB) PathMonitorData.NotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
    if (mask & MONITOR_MASK_NAME_CHANGE) PathMonitorData.NotifyFilter |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
    if (mask & MONITOR_MASK_CREATE) PathMonitorData.NotifyFilter |= FILE_NOTIFY_CHANGE_CREATION;
    if (mask & MONITOR_MASK_MODIFY) PathMonitorData.NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
    // Start monitoring

    if (!InternalStartMonitor(PathMonitorData)) {
        return NullHandle;
    }

    auto ResCompletionPort=CreateIoCompletionPort(DirHandle, CompletionPort, (uintptr_t)pPathMonitorData.get(), 0);//CancelIoEx
    if (ResCompletionPort != CompletionPort) {
        return NullHandle;
    }
    auto [itr,res]=MonitorDatas.try_emplace(CommonHandle_t(CommonHandle_t::atomic_count), pPathMonitorData);
    MonitorDataPathMap.try_emplace(ConvertStringToU8View(PathMonitorData.Path), itr->first);
    return itr->first;
}
void FFilesystemMonitorWin::CancelMonitor(CommonHandle_t handle) 
{
    auto itr=MonitorDatas.find(handle);
    if (itr == MonitorDatas.end()) {
        return;
    }
    auto& [key, data] = *itr;
    MonitorDataPathMap.erase(ConvertStringToU8View(data->Path));
    MonitorDatas.erase(itr);
}

void FFilesystemMonitorWin::Tick(float delta)
{
    ULONG numEntries = 0;
    const int MAX_ENTRIES = 10;
    OVERLAPPED_ENTRY entries[MAX_ENTRIES];
    BOOL success = GetQueuedCompletionStatusEx(
        CompletionPort,
        entries,
        MAX_ENTRIES,
        &numEntries,
        0, // 可设置超时
        FALSE
    );
    if (success == FALSE) {
        return;
    }
    for (int i = 0; i < numEntries; i++) {
        PathMonitorData_t& PathMonitorData =*(PathMonitorData_t*)entries[i].lpCompletionKey;
        PathMonitorData_t* pPathMonitorData=CONTAINING_RECORD(entries[i].lpOverlapped, PathMonitorData_t, OverlappedData);
        assert(pPathMonitorData == &PathMonitorData);
        FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)PathMonitorData.ResultBuf.data();
        while (true) {
            std::filesystem::path path(PathMonitorData.Path);
            path /= std::u16string_view((const char16_t*)pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR));
            EFilesystemAction action{ EFilesystemAction::FSA_CREATE };
            switch (pNotify->Action)
            {
            case FILE_ACTION_ADDED: {
                action = EFilesystemAction::FSA_CREATE;
                break;
            }
            case FILE_ACTION_REMOVED: {
                action = EFilesystemAction::FSA_DELETE;
                break;
            }
            case FILE_ACTION_MODIFIED: {
                action = EFilesystemAction::FSA_MODIFY;
                break;
            }
            case FILE_ACTION_RENAMED_OLD_NAME: {
                action = EFilesystemAction::FSA_RENAME;
                break;
            }
            case FILE_ACTION_RENAMED_NEW_NAME: {
                action = EFilesystemAction::FSA_RENAME;
                break;
            }
            }

            switch (pNotify->Action)
            {
            case FILE_ACTION_RENAMED_OLD_NAME: {
                pNotify = (FILE_NOTIFY_INFORMATION*)((BYTE*)pNotify + pNotify->NextEntryOffset);
                assert(pNotify->Action == FILE_ACTION_RENAMED_NEW_NAME);
                std::filesystem::path pathNew(PathMonitorData.Path);
                pathNew /= std::u16string_view((const char16_t*)pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR));
                PathMonitorData.Delegate(action, path.u8string(), pathNew.u8string());
                break;
            }
            default: {
                PathMonitorData.Delegate(action, path.u8string(), std::u8string_view());
            }
            }

            if (pNotify->NextEntryOffset == 0) break;
            pNotify = (FILE_NOTIFY_INFORMATION*)((BYTE*)pNotify + pNotify->NextEntryOffset);
        }
        InternalStartMonitor(PathMonitorData);
    }
}

bool FFilesystemMonitorWin::InternalStartMonitor(PathMonitorData_t& PathMonitorData)
{
    BOOL result = ReadDirectoryChangesW(
        PathMonitorData.DirHandle,
        PathMonitorData.ResultBuf.data(),
        PathMonitorData.ResultBuf.size(),
        TRUE,
        PathMonitorData.NotifyFilter,
        NULL,
        &PathMonitorData.OverlappedData,
        NULL
    );
    if (result == FALSE) {
        LPVOID buffer;
        if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&buffer,
            0,
            NULL))
        {
            auto message = std::string((const char*)buffer);
            LocalFree(buffer);
        }
        return false;
    }
    else {
        return true;
    }
}


IFilesystemMonitor* GetFilesystemMonitor()
{
    static std::atomic<std::shared_ptr<FFilesystemMonitorWin>> atomicptr;
    auto ptr = atomicptr.load();
    if (!ptr) {
        std::shared_ptr<FFilesystemMonitorWin> newptr(new FFilesystemMonitorWin);
        if (atomicptr.compare_exchange_strong(ptr, newptr)) {
            ptr = newptr;
        }
    }
    return ptr.get();
}