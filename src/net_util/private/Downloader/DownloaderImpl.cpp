#include "Downloader/DownloaderImpl.h"
#include "uri.h"
#include "HTTP/HttpManager.h"
#include "Downloader/download-file.capnp.h"
#include <dir_util.h>
#include <FunctionExitHelper.h>
#include <simple_math.h>
#include <simple_uuid.h>
#include <hex.h>
#include <singleton.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <CharBuffer.h>
#include <string_convert.h>
#include <simple_os_defs.h>
#include <chrono>
#include <LoggerHelper.h>
#include <thread>
#include <regex>
constexpr char DownloadTempExtensionStr[] = "tmp";
constexpr char DownloadDiskDataExtensionStr[] = "downloaddata";

// max size of a file chunk
static constexpr uint32_t DOWNLOAD_FILE_CHUNK_SIZE = 4 * 1024 * 1024;



class FDownloadFile;
class FDownloadBuf;
/**
* A struct present a chunk in file to download
*/
typedef struct file_chunk_t {
    uint32_t ChunkIndex{ 0 };
    uint32_t DownloadSize{ 0 };
    std::shared_ptr<FDownloadFile> File;
    std::shared_ptr<FDownloadBuf> Buf;
    char* BufCursor{ nullptr };
    std::pair<uint64_t, uint64_t> GetRange();
    uint64_t GetChunkSize() {
        return GetRange().second - GetRange().first;
    }
}file_chunk_t;

/**
* A class present a buf that many file chunk write to.
* Send to IO thread when all file chunk download complete.
*/
class  FDownloadBuf : public std::enable_shared_from_this<FDownloadBuf> {
public:

    FDownloadBuf(uint32_t length) {
        Len = length;
        Buf = (char*)malloc(Len);
    }
    ~FDownloadBuf() {
        if (Buf) {
            free(Buf);
        }
    }
    bool InsertInBuf(std::shared_ptr < file_chunk_t> pchunk);
    bool RemoveChunk(std::shared_ptr < file_chunk_t> pchunk);

    char* Buf{ nullptr };
    uint32_t Len;
    std::list<std::shared_ptr<file_chunk_t>> ChunkList;
};

/**
* A class present a file to be downloaded
*/
template<class T>
concept StringLike = std::is_convertible_v<T, std::string_view>;
class FDownloadFile : public std::enable_shared_from_this<FDownloadFile> {
public:

    FDownloadFile(std::string url, std::string folder);
    FDownloadFile(std::string url, FCharBuffer& content);
    FDownloadFile(std::string url, std::filesystem::path folder);

    ~FDownloadFile();
    void SetStatus(EFileTaskStatus status) {
        Status = status;
        auto DataBuilder = MessageBuilder.getRoot<DownloadFileDiskData>();
        DataBuilder.setStatus(std::to_underlying(Status));
    }
    bool Init();
    void UpdateOnlineFilename(std::string_view filename);
    void SetFileSize(int64_t size);
    std::shared_ptr<file_chunk_t> GetNotDownloadFilechunk();
    void RevertDownloadFilechunk(uint32_t ChunkIndex);
    bool IsAllChunkInDownload();
    uint64_t GetDownloadSize();
    bool IsIntact();
    void SaveRecoveryInfo();
    size_t SaveDate(void* ptr,int64_t len);
    size_t SaveDate(std::shared_ptr<file_chunk_t> file_chunk);
    bool CompleteChunk(std::shared_ptr<file_chunk_t> file_chunk);
    uint64_t GetChunkSize(uint32_t index);
    void CloseFileStream();

    template<StringLike T>
    void Finish(EDownloadCode err, T&& msg) {
        Status = EFileTaskStatus::Finished;
        Code = err;
        ErrorMsg= msg;
        CloseFileStream();
    }
    void Finish(EDownloadCode err) {
        Finish(err,"");
    }

    EFileTaskStatus Status{ EFileTaskStatus::Idle };
    EDownloadCode Code{ EDownloadCode::OK };
    std::string ID;
    std::string ErrorMsg;
    std::string URL;
    std::atomic_int64_t Size{ 0 };
    std::atomic_uint64_t DownloadSize{ 0 };
    std::atomic_uint64_t PreDownloadSize{ 0 };
    std::atomic_uint64_t LastTime;
    std::atomic_uint64_t PreTime;
    std::atomic_bool bTriggerProgressCB{ false };
    std::filesystem::path Path;
    std::filesystem::path WorkPath;

    FCharBuffer* Content{ nullptr };
    kj::Array<uint8_t> ChunksCompleteFlag;
    kj::Array<uint8_t> ChunksDownloadFlag;
    //std::vector<std::byte> ChunksCompleteFlag;
    //std::vector<std::byte> ChunksDownloadFlag;
    uint32_t ChunkNum{ 0 };
    capnp::MallocMessageBuilder MessageBuilder;
    std::unordered_map<HttpRequestPtr, std::shared_ptr<file_chunk_t>> Requests;
    bool bPause{ false };
    bool bRecoveryInfo{ true };
    FDownloadProgressDelegate DownloadProgressDelegate;
    FDownloadFinishedDelegate DownloadFinishedDelegate;
    FGetFileInfoDelegate GetFileInfoDelegate;
    FRawFile FileStream;
    FRawFile RecoveryInfoFile;
private:
    void GenProgressData(std::shared_ptr<file_chunk_t> file_chunk);

    FDownloadFile()=delete;
};



FDownloadFile::FDownloadFile(std::string url, std::filesystem::path folder) : Path(folder), URL(url){
}

FDownloadFile::FDownloadFile(std::string url, std::string path) : FDownloadFile(url, std::filesystem::path(path)) {}
FDownloadFile::FDownloadFile(std::string url, FCharBuffer& content) : Content(&content), URL(url),bRecoveryInfo(false) {

}

FDownloadFile::~FDownloadFile()
{
    if (FileStream.IsOpen()) {
        FileStream.Close();
    }
}

bool FDownloadFile::Init()
{
    if (Content) {
        return true;
    }
    if (!Path.is_absolute()) {
        Path = std::filesystem::current_path() / Path;
        Path = Path.lexically_normal();
    }
    if (WorkPath.empty()) {
        if (*Path.string().rbegin() == std::filesystem::path::preferred_separator) {
            WorkPath = Path;
        }
        else {
            WorkPath = Path.parent_path();
        }
    }
    auto& pathBuf = *FPathBuf::GetThreadSingleton();
    pathBuf.SetPathW(WorkPath.wstring());
    if (!DirUtil::CreateDir(pathBuf)) {
        return false;
    }

    if (ID.empty()) {
        auto& buf = *FCharBuffer::GetThreadSingleton();
        buf.Reverse(bin_to_hex_length(16));
        uint8_t uuidBuf[16];
        generate_uuid_128(uuidBuf);
        to_lower_hex(buf.Data(), uuidBuf, 16);
        ID.assign(buf.Data(), buf.Data() + bin_to_hex_length(16));
    }

    std::filesystem::path TempPath = WorkPath;
    TempPath.append(ID +"."+ DownloadTempExtensionStr);
    pathBuf.SetPathW(TempPath.wstring());
    if (FileStream.Open(pathBuf, UTIL_OPEN_ALWAYS, Size) != ERR_SUCCESS) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "open file failed path:{}", TempPath.string());
        return false;
    };

    std::filesystem::path RecoveryInfoPath = WorkPath;
    RecoveryInfoPath.append(ID + "." + DownloadDiskDataExtensionStr);
    pathBuf.SetPathW(RecoveryInfoPath.wstring());
    if (RecoveryInfoFile.Open(pathBuf, UTIL_OPEN_ALWAYS) != ERR_SUCCESS) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "open file failed path:{}", RecoveryInfoPath.string());
        return false;
    };

    auto DataBuilder = MessageBuilder.initRoot<DownloadFileDiskData>();
    DataBuilder.setUrl(URL);
    DataBuilder.setPath(ConvertU8ViewToString(Path.u8string()));
    return true;
}

void FDownloadFile::UpdateOnlineFilename(std::string_view filename)
{
    if (Content) {
        return;
    }

    if (*Path.string().rbegin() != std::filesystem::path::preferred_separator) {
        return;
    }
    Path.append(filename);
    auto DataBuilder = MessageBuilder.getRoot<DownloadFileDiskData>();
    DataBuilder.setPath(ConvertU8ViewToString(Path.u8string()));
}

void FDownloadFile::SetFileSize(int64_t size)
{
    Size.store(size);
    if (size > 0) {
        ChunkNum = CEIL_INTEGER(Size, DOWNLOAD_FILE_CHUNK_SIZE);
        auto byteNum = CEIL_INTEGER(ChunkNum, CHAR_BIT);
        ChunksCompleteFlag = kj::heapArray<uint8_t>(byteNum);
        ChunksDownloadFlag = kj::heapArray<uint8_t>(byteNum);
        for (int i = 0; i < byteNum;i++) {
            ChunksCompleteFlag[i] = 0;
            ChunksDownloadFlag[i] = 0;
        }
        if (Content) {
            Content->Reverse(size + 1);
            Content->SetLength(size);
        }
        else {
            auto DataBuilder = MessageBuilder.getRoot<DownloadFileDiskData>();
            DataBuilder.setSize(size);
            DataBuilder.setChunkNum(ChunkNum);
            DataBuilder.setChunksCompleteFlag(ChunksCompleteFlag);
        }
    }
}

std::shared_ptr<file_chunk_t> FDownloadFile::GetNotDownloadFilechunk()
{
    std::shared_ptr<FDownloadFile> my;
    try {
      my = shared_from_this();
    }
    catch (std::bad_weak_ptr& e) {
        SIMPLELOG_LOGGER_ERROR(nullptr,"GetNotDownloadFilechunk without shared_ptr");
        return nullptr;
    }
    auto pchunk=std::make_shared<file_chunk_t>();
    pchunk->File = my;
    for (int i = 0; i < ChunksDownloadFlag.size()-1; i++) {
        if (ChunksDownloadFlag[i] == UCHAR_MAX) {
            continue;
        }
        for (uint32_t j = 0; j < CHAR_BIT; j++) {
            auto mask=uint8_t(1) << j;
            if ((ChunksDownloadFlag[i] & mask) == 0) {
                ChunksDownloadFlag[i] |= mask;
                pchunk->ChunkIndex = i * CHAR_BIT + j;
                return pchunk;
            }
        }
    }
    auto lastnum = ChunkNum % CHAR_BIT;
    lastnum = lastnum == 0 ? CHAR_BIT : lastnum;
    auto& lastbyte = ChunksDownloadFlag.back();
    for (uint32_t j = 0; j < lastnum; j++) {
        auto mask = uint8_t(1) << j;
        if ((lastbyte & mask) == 0) {
            lastbyte |= mask;
            pchunk->ChunkIndex = ChunkNum - lastnum + j;
            return pchunk;
        }
    }
    return nullptr;
}

void FDownloadFile::RevertDownloadFilechunk(uint32_t ChunkIndex)
{
    auto j = ChunkIndex % CHAR_BIT;
    auto i = ChunkIndex / CHAR_BIT;
    auto mask = ~(uint8_t(1) << j);
    ChunksDownloadFlag[i] &= mask;
}

bool FDownloadFile::IsAllChunkInDownload()
{
    auto lastnum = ChunkNum % CHAR_BIT;
    lastnum = lastnum == 0 ? CHAR_BIT : lastnum;
    auto lastbyte = ChunksDownloadFlag.back();
    if (lastbyte == UCHAR_MAX >> (CHAR_BIT - lastnum)) {
        return true;
    }
    return false;
}

uint64_t FDownloadFile::GetDownloadSize() {
    uint64_t DownloadSize;
    for (uint32_t i = 0; i < ChunksCompleteFlag.size() - 1; i++) {
        auto s = ChunksCompleteFlag[i];
        if (ChunksCompleteFlag[i] == UCHAR_MAX) {
            DownloadSize += DOWNLOAD_FILE_CHUNK_SIZE * 8;
        }
        for (uint32_t j = 0; j < CHAR_BIT; j++) {
            auto mask = uint8_t(1) << j;
            if ((ChunksCompleteFlag[i] & mask) != 0) {
                DownloadSize += DOWNLOAD_FILE_CHUNK_SIZE;
            }
        }
    }
    auto lastnum = ChunkNum % CHAR_BIT;
    lastnum = lastnum == 0 ? CHAR_BIT : lastnum;
    auto& lastbyte = ChunksCompleteFlag.back();
    for (uint32_t j = 0; j < lastnum; j++) {
        auto mask = uint8_t(1) << j;

        if ((lastbyte & mask) != 0) {
            DownloadSize += DOWNLOAD_FILE_CHUNK_SIZE;
        }
    }
    return DownloadSize;
}
bool FDownloadFile::IsIntact()
{
    return Size == GetDownloadSize();
}


size_t FDownloadFile::SaveDate(void* ptr, int64_t len)
{
    if (Content) {
        Content->Append((char*)ptr, len);
    }
    else {
        int32_t res;
        res = FileStream.Write(ptr, len);
        if (res != ERR_SUCCESS) {
            return 0;
        }
    }
    return len;
}

size_t FDownloadFile::SaveDate(std::shared_ptr<file_chunk_t>  file_chunk)
{
    auto range = file_chunk->GetRange();
    auto& [beginIdx, endIdx] = range;
    if (Content) {
        memcpy(Content->Data() + beginIdx, file_chunk->BufCursor, file_chunk->GetChunkSize());
    }
    else {
        int32_t res;
        res = FileStream.Seek(beginIdx);
        if (res != ERR_SUCCESS) {
            return 0;
        }
        res = FileStream.Write(file_chunk->BufCursor, file_chunk->GetChunkSize());
        if (res != ERR_SUCCESS) {
            return 0;
        }
        FileStream.Flush();
    }
    return  file_chunk->GetChunkSize();
}



bool FDownloadFile::CompleteChunk(std::shared_ptr<file_chunk_t> file_chunk)
{
    assert(file_chunk->File.get() == this );
    ChunksCompleteFlag[file_chunk->ChunkIndex / 8] |= (uint8_t(1) << file_chunk->ChunkIndex % 8);
    GenProgressData(file_chunk);
    auto DataBuilder = MessageBuilder.getRoot<DownloadFileDiskData>();
    DataBuilder.setChunksCompleteFlag(ChunksCompleteFlag);
    DataBuilder.setDownloadSize(DownloadSize);
    return true;
}

uint64_t FDownloadFile::GetChunkSize(uint32_t index)
{
    if (Size / DOWNLOAD_FILE_CHUNK_SIZE > index) {
        return DOWNLOAD_FILE_CHUNK_SIZE;
    }
    return Size % DOWNLOAD_FILE_CHUNK_SIZE;
}

void FDownloadFile::CloseFileStream()
{
    if (FileStream.IsOpen()) {
        FileStream.Close();
    }
    if (RecoveryInfoFile.IsOpen()) {
        RecoveryInfoFile.Close();
    }

    if (bRecoveryInfo) {
        if (DownloadSize == Size) {
            DirUtil::Rename((const char8_t*)FileStream.GetFilePath(), Path.u8string());
            DirUtil::Delete((const char8_t*)RecoveryInfoFile.GetFilePath());
        }
    }

}

void FDownloadFile::GenProgressData(std::shared_ptr<file_chunk_t> chunk)
{
    auto presize = DownloadSize.fetch_add(chunk->GetChunkSize());
    PreDownloadSize.store(presize);
    auto now = std::chrono::steady_clock::now();
    auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    auto prestamp = LastTime.exchange(stamp);
    PreTime.store(prestamp);
    bTriggerProgressCB = true;
}

void FDownloadFile::SaveRecoveryInfo()
{
    if (!bRecoveryInfo) {
        return;
    }
    auto DataBuilder = MessageBuilder.getRoot<DownloadFileDiskData>();
    capnp::writePackedMessageToFd((int)RecoveryInfoFile.GetFD(), MessageBuilder);
}

bool FDownloadBuf::InsertInBuf(std::shared_ptr < file_chunk_t> pchunk) {
    std::shared_ptr<FDownloadBuf> my;
    try {
        my = shared_from_this();
    }
    catch (std::bad_weak_ptr& e) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "InsertInBuf without shared_ptr");
        return false;
    }
    auto range = pchunk->GetRange();
    auto len = range.second - range.first;
    pchunk->Buf = my;
    auto cursor = Buf;
    for (auto itr = ChunkList.begin(); itr != ChunkList.end(); std::advance(itr, 1)) {
        auto& pold_chunk = *itr;
        auto old_range = pold_chunk->GetRange();
        if (pold_chunk->BufCursor - cursor >= len) {
            ChunkList.insert(itr, pchunk);
            pchunk->BufCursor = cursor;
            return true;
        }
        cursor = pold_chunk->BufCursor + old_range.second - old_range.first;
    }
    if (Buf + Len - cursor >= len) {
        ChunkList.push_back(pchunk);
        pchunk->BufCursor = cursor;
        return true;
    }
    return false;
}

bool FDownloadBuf::RemoveChunk(std::shared_ptr<file_chunk_t> pchunk)
{
    auto itr=std::find(ChunkList.cbegin(), ChunkList.cend(), pchunk);
    if (itr == ChunkList.cend()) {
        return false;
    }
    ChunkList.erase(itr);
    pchunk->Buf.reset();
    pchunk->BufCursor = nullptr;
    return true;
}

IDownloader* IDownloader::Instance()
{
    return FDownloader::Instance();
}

FDownloader::FDownloader(){
    pHttpManager = IHttpManager::GetNamedClassSingleton(CURL_HTTP_MANAGER_NAME);
}

std::shared_ptr<TaskStatus_t> FDownloader::GetTaskStatus(std::shared_ptr<FDownloadFile> pfile)
{
    auto& status = *OutStatus;

    status.DownloadSize = pfile->DownloadSize.load();
    status.PreDownloadSize = pfile->PreDownloadSize.load();
    status.LastTime = pfile->LastTime.load();
    status.PreTime = pfile->PreTime.load();
    status.IsCompelete = pfile->Status == EFileTaskStatus::Finished;

    status.ChunksCompleteFlag.resize(pfile->ChunksCompleteFlag.size());
    for (int i = 0; i < pfile->ChunksCompleteFlag.size(); i++) {
        status.ChunksCompleteFlag[i] = pfile->ChunksCompleteFlag[i];
    }
    return OutStatus;
}

std::shared_ptr<DownloadFileInfo> FDownloader::GetTaskInfo(std::shared_ptr<FDownloadFile> pfile) {
    auto& info = *OutFileInfo;
    info.FilePath = pfile->Path;
    info.FileSize = pfile->Size;
    info.ChunkNum = pfile->ChunkNum;
    return OutFileInfo;
}
DownloadTaskHandle_t FDownloader::AddTask(FDownloadFile* file)
{
    for (auto& [handle,pfile] : Files) {
        if (pfile->URL == file->URL ) {
            return handle;
        }
    }
    auto respair=Files.try_emplace(CommonHandle32_t(DownloadTaskHandle_t::task_count), file);
    if (respair.second) {
        file->Init();
        file->SaveRecoveryInfo();
        return respair.first->first;
    }
    return DownloadTaskHandle_t();
}

void FDownloader::TransferBuf()
{

    for (auto pool_itr = BufPool.begin(); pool_itr != BufPool.end();) {
        auto& pbuf = *pool_itr;
        auto itr = pbuf->ChunkList.begin();
        for (; itr != pbuf->ChunkList.end(); std::advance(itr, 1)) {
            auto& pchunk = *itr;
            auto range=pchunk->GetRange();
            if (pchunk->DownloadSize != range.second-range.first) {
                break;
            }
        }
        if (itr == pbuf->ChunkList.end()&& pbuf->ChunkList.size()>0) {
            BufInIO.enqueue(pbuf);
            pool_itr=BufPool.erase(pool_itr);
        }
        else {
            std::advance(pool_itr, 1);
        }
    }

    do {
        auto count = BufIOComplete.try_dequeue_bulk(BufIOCompleteBuf, BUF_NUM);
        if (count == 0) {
            break;
        }
        for (int i = 0; i < count; i++) {
            auto& buf = BufIOCompleteBuf[i];
            buf->ChunkList.clear();
            BufPool.push_back(buf);
        }
    } while (true);
}

std::shared_ptr<FDownloadBuf> FDownloader::InsertInBuf(std::shared_ptr < file_chunk_t> chunk)
{
    for (auto& pbuf : BufPool) {
        auto res=pbuf->InsertInBuf(chunk);
        if (res) {
            return pbuf;
        }
    }
    return nullptr;
}

void FDownloader::StartDownloadWithoutContentLength(std::shared_ptr<FDownloadFile> pfile)
{
    auto& HttpManager = *pHttpManager;
    auto req = HttpManager.NewRequest();
    req->SetURL(pfile->URL);
    req->SetVerb(VERB_GET);
    req->SetHeader("User-Agent", "Downloader");
    req->EnableRespContent(false);
    req->OnHttpThreadRespContentReceive() = [pfile](HttpRequestPtr, int64_t oldSize, int64_t newSize, void* ptr, int64_t len) {
        pfile->DownloadSize = newSize;
        pfile->PreDownloadSize = oldSize;
        auto now = std::chrono::steady_clock::now();
        auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        auto prestamp = pfile->LastTime.exchange(stamp);
        pfile->PreTime.store(prestamp);
        pfile->SaveDate(ptr, len);
        pfile->bTriggerProgressCB = true;
        };
    req->OnProcessRequestComplete() = [pfile](HttpRequestPtr req, HttpResponsePtr resp, bool res) {
        if (!res) {
            pfile->Finish(EDownloadCode::SERVER_ERROR, "net failed");
            return;
        }
        pfile->Finish(EDownloadCode::OK);
        };
    HttpManager.ProcessRequest(req);
    pfile->Requests.try_emplace(req, nullptr);
}

FDownloader* FDownloader::Instance() {
    static std::atomic<FDownloader*> atomic_downloader;
    FDownloader*  downloader = atomic_downloader.load();
    if (!downloader) {
        FDownloader* expect{ nullptr };
        if (atomic_downloader.compare_exchange_strong(expect, new FDownloader())) {
            downloader = atomic_downloader.load();
            for (int i = 0; i < BUF_NUM; i++) {
                auto ptr = std::make_shared<FDownloadBuf>(BUF_CHUNK_NUM * DOWNLOAD_FILE_CHUNK_SIZE);
                downloader->BufPool.push_back(ptr);
                if (downloader->BufPool.size() <= i) {
                    delete downloader;
                    break;
                }
                if (!downloader->BufPool.back()->Buf) {
                    delete downloader;
                    break;
                }
            }
            downloader->OutStatus = std::make_shared<TaskStatus_t>();
            downloader->OutFileInfo = std::make_shared<DownloadFileInfo_t>();
        }
    }
    
    return atomic_downloader.load();
}
FDownloader::~FDownloader()
{
}
FDownloadTaskIterator FDownloader::Begin() {
    FDownloadTaskIterator outitr;
    auto internalItr = std::make_unique<FIterator>(this);
    internalItr->Itr = Files.begin();
    outitr.itr = std::move(internalItr);
    return outitr;
}
FDownloadTaskIterator FDownloader::End() {
    FDownloadTaskIterator outitr;
    auto internalItr = std::make_unique<FIterator>(this);
    internalItr->Itr = Files.end();
    outitr.itr = std::move(internalItr);
    return outitr;
}
//DownloadTaskHandle FDownloader::AddTask(std::string url, std::string path) {
//    FDownloadFile* df = new FDownloadFile(url, path);
//    return AddTask(df);
//}

DownloadTaskHandle_t FDownloader::AddTask(std::u8string_view url, FCharBuffer& contentBuf)
{
    FDownloadFile* df = new FDownloadFile(ConvertU8ViewToString(url), contentBuf);
    return AddTask(df);
}

DownloadTaskHandle_t FDownloader::AddTask(std::u8string_view url, const std::filesystem::path& folder)
{
    FDownloadFile* df = new FDownloadFile(ConvertU8ViewToString(url), folder);
    return AddTask(df);
}

void FDownloader::LoadDiskTask(std::u8string_view pathStr)
{
    auto& pathBuf = *FPathBuf::GetThreadSingleton();
    pathBuf.SetPath(ConvertU8ViewToView(pathStr));
    DirUtil::IterateDir(pathBuf,
        [this, pathStr](DirEntry_t& entry)->bool {
            auto& pathBuf=*entry.pPathBuf;
            std::filesystem::path filePath= pathBuf.FileNameW();
            if (!filePath.extension().string().contains(DownloadDiskDataExtensionStr)) {
                return true;
            }
            auto& file = *FRawFile::GetThreadSingleton();
            if (file.Open(pathBuf, UTIL_OPEN_EXISTING)!= ERR_SUCCESS) {
                return false;
            }
            auto diskDataPath = filePath.replace_extension(DownloadDiskDataExtensionStr);
            if (file.GetSize() == 0||!DirUtil::IsExist(diskDataPath.u8string())) {
                file.Close();
                DirUtil::Delete(filePath.u8string());
                DirUtil::Delete(diskDataPath.u8string());
                return true;
            }
            ::capnp::PackedFdMessageReader message((int)file.GetFD());
            DownloadFileDiskData::Reader downloadFileDiskData = message.getRoot<DownloadFileDiskData>();
            FDownloadFile* df = new FDownloadFile(downloadFileDiskData.getUrl(), downloadFileDiskData.getPath());
            df->Status = EFileTaskStatus(downloadFileDiskData.getStatus());
            df->ChunkNum = downloadFileDiskData.getChunkNum();
            df->DownloadSize = downloadFileDiskData.getDownloadSize();
            df->ChunksCompleteFlag = kj::heapArray<uint8_t>(downloadFileDiskData.getChunksCompleteFlag().size());
            df->ChunksDownloadFlag = kj::heapArray<uint8_t>(downloadFileDiskData.getChunksCompleteFlag().size());

            df->Size=downloadFileDiskData.getSize();
            df->Path=downloadFileDiskData.getPath().cStr();
            df->URL = downloadFileDiskData.getUrl().cStr();
            for (int i = 0; i < downloadFileDiskData.getChunksCompleteFlag().size();i++) {
                df->ChunksCompleteFlag[i] = downloadFileDiskData.getChunksCompleteFlag()[i];
                df->ChunksDownloadFlag[i] = downloadFileDiskData.getChunksCompleteFlag()[i];
            }
            file.Close();
            df->WorkPath = pathStr;
            df->ID = filePath.stem().string();
            df->Init();
            auto respair = Files.try_emplace(CommonHandle32_t(DownloadTaskHandle_t::task_count), df);
            if (respair.second) {
                return respair.first->first;
            }
            return true;
        },
        1);
}

void FDownloader::RemoveTask(DownloadTaskHandle_t handle)
{
    auto itr = Files.find(handle);
    if (itr == Files.end()) {
        return;
    }
    RequireRemoveFiles.insert(itr->first);
}

bool FDownloader::RegisterDownloadProgressDelegate(DownloadTaskHandle_t handle, FDownloadProgressDelegate Delegate)
{
    auto itr = Files.find(handle);
    if (itr == Files.end()) {
        return false;
    }
    itr->second->DownloadProgressDelegate = Delegate;
    return true;
}

bool FDownloader::RegisterDownloadFinishedDelegate(DownloadTaskHandle_t handle, FDownloadFinishedDelegate Delegate)
{
    auto itr = Files.find(handle);
    if (itr == Files.end()) {
        return false;
    }
    itr->second->DownloadFinishedDelegate = Delegate;
    return true;
}

bool FDownloader::RegisterGetFileInfoDelegate(DownloadTaskHandle_t handle, FGetFileInfoDelegate Delegate)
{
    auto itr = Files.find(handle);
    if (itr == Files.end()) {
        return false;
    }
    itr->second->GetFileInfoDelegate = Delegate;
    return true;
}

std::shared_ptr<TaskStatus_t> FDownloader::GetTaskStatus(DownloadTaskHandle_t handle) {
    auto itr = Files.find(handle);
    if (itr == Files.end()) {
        return nullptr;
    }
  
    auto& pfile = itr->second;
    return GetTaskStatus(pfile);

}

std::shared_ptr<DownloadFileInfo> FDownloader::GetTaskInfo(DownloadTaskHandle_t handle)
{
    auto itr = Files.find(handle);
    if (itr == Files.end()) {
        return nullptr;
    }

    auto& pfile = itr->second;
    return GetTaskInfo(pfile);
}

void FDownloader::Tick(float delSec)
{
    auto& HttpManager = *pHttpManager;
    TransferBuf();
    for (auto itr = RequireRemoveFiles.begin(); itr != RequireRemoveFiles.end(); std::advance(itr,1)) {
        Files.erase(*itr);
    }
    RequireRemoveFiles.clear();
    for (auto& [handle, pfile] : Files) {
        switch (pfile->Status) {
        case EFileTaskStatus::Idle: {
            auto preq = HttpManager.NewRequest();
            preq->SetURL(pfile->URL);
            preq->SetVerb(VERB_HEAD);
            preq->SetHeader("User-Agent", "Downloader");
            auto pweakFile = pfile->weak_from_this();
            preq->OnProcessRequestComplete() =
                [this, pweakFile, handle](HttpRequestPtr req, HttpResponsePtr rep, bool res) {
                auto pfile = pweakFile.lock();
                if (!pfile) {
                    return;
                }
                if (!res || rep->GetContentLength() == 0) {
                    pfile->Finish(EDownloadCode::SERVER_ERROR, "net error");
                    return;
                }
                pfile->SetFileSize(rep->GetContentLength());
                pfile->URL = rep->GetURL();

                auto str = std::string(rep->GetHeader("Content-Disposition"));
                if (!str.empty()) {
                    try {
                        std::regex filename_reg(
                            R"_(^[^]*filename="([^\/?#\"]+)"[^]*)_",
                            std::regex::ECMAScript
                        );
                        std::smatch match_result;
                        if (!std::regex_match(str, match_result, filename_reg)) {
                            SIMPLELOG_LOGGER_ERROR(nullptr, "filename parse error");
                            pfile->Finish(EDownloadCode::SERVER_ERROR, "filename parse error");
                            return;
                        }
                        pfile->UpdateOnlineFilename(match_result[1].str());
                    }
                    catch (std::exception& ex) {
                        SIMPLELOG_LOGGER_ERROR(nullptr, "{}", ex.what());
                        pfile->Finish(EDownloadCode::SERVER_ERROR, "regex exception");
                        return;
                    }
                }
                else {
                    ParsedURL_t parseRes;
                    ParseUrl(pfile->URL, parseRes);
                    pfile->UpdateOnlineFilename(parseRes.outPath);
                }

                if (pfile->GetFileInfoDelegate) {
                    pfile->GetFileInfoDelegate(handle, GetTaskInfo(pfile));
                }
                if (!pfile->Content) {
                    auto& pathBuf = *FPathBuf::GetThreadSingleton();
                    pathBuf.SetPathW(pfile->Path.wstring());
                    if (DirUtil::IsExist(pathBuf)) {
                        pfile->Finish(EDownloadCode::OK, "file exist");
                        return;
                    }
                }
                if (pfile->Size > 0) {
                    pfile->SetStatus(EFileTaskStatus::DownloadChunk);
                    pfile->SaveRecoveryInfo();
                }
                else {
                    pfile->SetStatus(EFileTaskStatus::Download);
                    StartDownloadWithoutContentLength(pfile);
                }
                };
            HttpManager.ProcessRequest(preq);
            pfile->SetStatus(EFileTaskStatus::Init);
            break;
        }
        case EFileTaskStatus::DownloadChunk: {
            while (true) {
                if (pfile->DownloadProgressDelegate) {
                    if (pfile->bTriggerProgressCB.exchange(false)) {
                        pfile->DownloadProgressDelegate(handle, GetTaskStatus(pfile));
                    }
                }
                if (pfile->DownloadSize == pfile->Size) {
                    pfile->Finish(EDownloadCode::OK);
                }
                if (pfile->IsAllChunkInDownload()) {
                    break;
                }
                if (pfile->Requests.size() >= ParallelChunkPerFile) {
                    break;
                }
                auto pchunk = pfile->GetNotDownloadFilechunk();
                if (!pchunk) {
                    SIMPLELOG_LOGGER_ERROR(nullptr, "Downloader GetNotDownloadFilechunk Error");
                    break;
                }
                std::shared_ptr<FDownloadBuf> buf;
                buf = InsertInBuf(pchunk);
                if (!buf) {
                    pfile->RevertDownloadFilechunk(pchunk->ChunkIndex);
                    break;
                }

                auto req = HttpManager.NewRequest();
                req->SetURL(pfile->URL);
                req->SetVerb(VERB_GET);
                req->SetHeader("User-Agent", "Downloader");
                auto range = pchunk->GetRange();
                req->SetRange(range.first, range.second - 1);
                req->GetResponse()->SetContentBuf(pchunk->BufCursor, pchunk->GetChunkSize());
                std::weak_ptr<file_chunk_t> pweakChunk = pchunk;
                req->OnRequestProgress() = [pweakChunk](HttpRequestPtr req, int64_t oldSize, int64_t newSize, int64_t totalSize) {
                    auto pchunk = pweakChunk.lock();
                    if (!pchunk) {
                        return;
                    }
                    pchunk->DownloadSize = newSize;
                    };
                req->OnProcessRequestComplete() = [pfile](HttpRequestPtr req, HttpResponsePtr resp, bool res) {
                    auto itr = pfile->Requests.find(req);
                    if (itr == pfile->Requests.end()) {
                        return;
                    }
                    auto [_, pchunk] = *itr;
                    pfile->Requests.erase(itr);
                    if (!res) {
                        pchunk->File->RevertDownloadFilechunk(pchunk->ChunkIndex);
                        pchunk->Buf->RemoveChunk(pchunk);
                        return;
                    }
                    if (resp->GetContentBytesRead() != pchunk->GetChunkSize()) {
                        pchunk->File->RevertDownloadFilechunk(pchunk->ChunkIndex);
                        pchunk->Buf->RemoveChunk(pchunk);
                        SIMPLELOG_LOGGER_ERROR(nullptr, "Downloaded size not equal range");
                        return;
                    }
                    req->OnRequestProgress() = nullptr;
                    pchunk->DownloadSize = resp->GetContentBytesRead();
                    };
                HttpManager.ProcessRequest(req);
                pfile->Requests.try_emplace(req, pchunk);
            }

            break;

        }
        case EFileTaskStatus::Download: {
            break;
        }
        case EFileTaskStatus::Finished: {
            if (pfile->DownloadFinishedDelegate) {
                pfile->DownloadFinishedDelegate(DownloadTaskHandle(handle), pfile->Code);
                pfile->DownloadFinishedDelegate = nullptr;
            }
            break;
        }
        }
    }
}


void FDownloader::IOThreadTick(float delSec)
{
    do {
        auto count = BufInIO.try_dequeue_bulk(BufInIOBuf, BUF_NUM);
        if (count == 0) {
            break;
        }
        for (int i = 0; i < count; i++) {
            auto& buf = BufInIOBuf[i];
            for (auto itr = buf->ChunkList.begin(); itr != buf->ChunkList.end();) {
                auto const& chunk = *itr;
                auto& pFile = chunk->File;
                if (pFile->SaveDate(chunk) == chunk->GetChunkSize()) {
                    pFile->CompleteChunk(chunk);
                    pFile->SaveRecoveryInfo();
                    itr = buf->ChunkList.erase(itr);
                }
                else {
                    SIMPLELOG_LOGGER_ERROR(nullptr, "Downloader SavaDate Error");
                    std::advance(itr, 1);
                }
            }
            BufIOComplete.enqueue(buf);
        }

    } while (true);
}

std::pair<uint64_t, uint64_t> file_chunk_t::GetRange()
{
    auto begin = int64_t(DOWNLOAD_FILE_CHUNK_SIZE) * ChunkIndex;
    auto end = begin + DOWNLOAD_FILE_CHUNK_SIZE;
    auto filesize = File->Size.load();
    return std::pair(begin, std::min(end, filesize));
}
