#include "Downloader/downloader.h"
#include <simple_os_defs.h>
#include <chrono>
#include <LoggerHelper.h>
#include <thread>
#include <regex>
#include <handle.h>

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



class FDownloadFile : public std::enable_shared_from_this<FDownloadFile> {
public:

    FDownloadFile(std::string url, std::string folder);
    FDownloadFile(std::string url, std::string* content);
    FDownloadFile(std::string url, std::filesystem::path folder);

    ~FDownloadFile();
    void SetFileSize(uint64_t size);
    std::shared_ptr<file_chunk_t> GetNotDownloadFilechunk();
    void RevertDownloadFilechunk(uint32_t ChunkIndex);
    bool IsAllChunkInDownload();
    uint64_t GetDownloadSize();
    bool IsIntact();

    size_t SavaDate(std::shared_ptr<file_chunk_t> file_chunk);
    bool CompleteChunk(std::shared_ptr<file_chunk_t> file_chunk);
    uint64_t GetChunkSize(uint32_t index);
    void CloseFileStream();
    void Finish(EDownloadCode err, std::string&& msg) {
        Finish(err);
        ErrorMsg = std::forward<std::string>(msg);
    }
    void Finish(EDownloadCode err) {
        Status = EFileTaskStatus::Finished;
        Code = err;
        ErrorMsg.clear();
    }

    //FDownloadFile& operator=(const FDownloadFile& other) = delete;
    //FDownloadFile(FDownloadFile&) = delete;
    //FDownloadFile& operator=(const FDownloadFile&& other) = delete;
    //FDownloadFile(FDownloadFile&&) = delete;

    static const uint32_t CHUNK_SIZE;
    EFileTaskStatus Status{ EFileTaskStatus::Idle };
    EDownloadCode Code{ EDownloadCode::OK };
    std::string ErrorMsg;
    std::string URL;
    std::atomic_int64_t Size{ 0 };
    std::atomic_uint64_t DownloadSize{ 0 };
    std::atomic_uint64_t PreDownloadSize{ 0 };
    std::atomic_uint64_t LastTime;
    std::atomic_uint64_t PreTime;
    std::filesystem::path Path;
    std::string* Content{ nullptr };
    std::vector<std::byte> ChunksCompleteFlag;
    std::vector<std::byte> ChunksDownloadFlag;
    uint32_t ChunkNum{ 0 };
    std::set<HttpRequestPtr> RequestPool;
    std::unordered_map<HttpRequestPtr, file_chunk_t> Requests;

    FDownloadProgressDelegate DownloadProgressDelegate;
    FDownloadFinishedDelegate DownloadFinishedDelegate;
    FGetFileInfoDelegate GetFileInfoDelegate;
private:
    bool Open();
    CRawFile FileStream;
    FDownloadFile();
};

typedef struct file_chunk_s {
    uint32_t ChunkIndex{ 0 };
    uint32_t DownloadSize{ 0 };
    std::shared_ptr<FDownloadFile> File;
    std::shared_ptr<FDownloadBuf> Buf;
    char* BufCursor{ nullptr };
    std::pair<uint64_t, uint64_t> GetRange() {
        auto begin = FDownloadFile::CHUNK_SIZE * ChunkIndex;
        auto end = begin + FDownloadFile::CHUNK_SIZE - 1;
        auto filesize = File->Size.load();
        return std::pair(begin, end > filesize - 1 ? filesize - 1 : end + 1);
    }
    uint64_t GetChunkSize() {
        return GetRange().second - GetRange().first + 1;
    }
}file_chunk_t;


const uint32_t FDownloadFile::CHUNK_SIZE = 4*1024*1024;
const uint32_t FDownloader::BUF_NUM = 4;
const uint32_t FDownloader::BUF_CHUNK_NUM = 3;


std::atomic_uint32_t DownloadTaskHandle_t::task_count;

FDownloadFile::FDownloadFile(std::string url, std::string path) : Path(path), URL(url){}
FDownloadFile::FDownloadFile(std::string url, std::string* content) : Content(content), URL(url) {}

FDownloadFile::FDownloadFile(std::string url, std::filesystem::path folder) : Path(folder), URL(url) {}
FDownloadFile::FDownloadFile() {}
FDownloadFile::~FDownloadFile()
{
    if (FileStream.IsOpen()) {
        FileStream.Close();
    }
}

void FDownloadFile::SetFileSize(uint64_t size)
{
    Size.store(size);
    ChunkNum = Size / FDownloadFile::CHUNK_SIZE + (Size % FDownloadFile::CHUNK_SIZE > 0 ? 1 : 0);
    for (int i = 0; i * CHAR_BIT < ChunkNum; i++) {
        ChunksCompleteFlag.push_back(std::byte{ 0 });
        ChunksDownloadFlag.push_back(std::byte{ 0 });
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
    for (uint32_t i = 0; i < ChunksDownloadFlag.size()-1; i++) {
        if (ChunksDownloadFlag[i] == std::byte(UCHAR_MAX)) {
            continue;
        }
        for (uint32_t j = 0; j < CHAR_BIT; j++) {
            auto mask=std::byte(1) << j;
            if ((ChunksDownloadFlag[i] & mask) == std::byte(0)) {
                ChunksDownloadFlag[i] |= mask;
                pchunk->ChunkIndex = i * CHAR_BIT + j;
                return pchunk;
            }
        }
    }
    auto lastnum = ChunkNum % CHAR_BIT;
    auto& lastbyte = ChunksDownloadFlag.back();
    for (uint32_t j = 0; j < lastnum; j++) {
        auto mask = std::byte(1) << j;

        if ((lastbyte & mask) == std::byte(0)) {
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
    auto mask = ~(std::byte(1) << j);
    ChunksDownloadFlag[i] &= mask;
}

bool FDownloadFile::IsAllChunkInDownload()
{
    auto lastnum = ChunkNum % CHAR_BIT;
    auto lastbyte = ChunksDownloadFlag.back();
    if (lastbyte == std::byte(UCHAR_MAX) >> (CHAR_BIT - lastnum)) {
        return true;
    }
    return false;
}

uint64_t FDownloadFile::GetDownloadSize() {
    uint64_t DownloadSize;
    for (uint32_t i = 0; i < ChunksCompleteFlag.size() - 1; i++) {
        if (ChunksCompleteFlag[i] == std::byte(UCHAR_MAX)) {
            DownloadSize += FDownloadFile::CHUNK_SIZE * 8;
        }
        for (uint32_t j = 0; j < CHAR_BIT; j++) {
            auto mask = std::byte(1) << j;
            if ((ChunksCompleteFlag[i] & mask) != std::byte(0)) {
                DownloadSize += FDownloadFile::CHUNK_SIZE;
            }
        }
    }
    auto lastnum = ChunkNum % CHAR_BIT;
    auto& lastbyte = ChunksCompleteFlag.back();
    for (uint32_t j = 0; j < lastnum; j++) {
        auto mask = std::byte(1) << j;

        if ((lastbyte & mask) != std::byte(0)) {
            DownloadSize += FDownloadFile::CHUNK_SIZE;
        }
    }
    return DownloadSize;
}
bool FDownloadFile::IsIntact()
{
    return Size == GetDownloadSize();
}


size_t FDownloadFile::SavaDate(std::shared_ptr<file_chunk_t>  file_chunk)
{
    if (!Open()) {
        return 0;
    }
    auto range = file_chunk->GetRange();
    int32_t res;
    res=FileStream.Seek(range.first);
    if (res!= ERR_SUCCESS) {
        return 0;
    }
    res=FileStream.Write(file_chunk->BufCursor, file_chunk->GetChunkSize());
    if (res != ERR_SUCCESS) {
        return 0;
    }
    return  file_chunk->GetChunkSize();
}



bool FDownloadFile::CompleteChunk(std::shared_ptr<file_chunk_t> file_chunk)
{
    assert(file_chunk->File.get() == this );
    ChunksCompleteFlag[file_chunk->ChunkIndex / 8] |= (std::byte(1) << file_chunk->ChunkIndex % 8);
    return true;
}

uint64_t FDownloadFile::GetChunkSize(uint32_t index)
{
    if (Size / FDownloadFile::CHUNK_SIZE > index) {
        return FDownloadFile::CHUNK_SIZE;
    }
    return Size % FDownloadFile::CHUNK_SIZE;
}

void FDownloadFile::CloseFileStream()
{
    if (FileStream.IsOpen()) {
        FileStream.Close();
    }
}

bool FDownloadFile::Open()
{
    if (!FileStream.IsOpen()) {
        if (!Path.is_absolute()) {
            Path = std::filesystem::current_path() / Path;
            Path = Path.lexically_normal();
        }

        if (!std::filesystem::exists(Path.parent_path())) {
            std::filesystem::create_directories(Path.parent_path());
        }
        if (FileStream.Open(Path.u8string().c_str(), UTIL_OPEN_ALWAYS, Size)!= ERR_SUCCESS) {
            SIMPLELOG_LOGGER_ERROR(nullptr, "open file failed path:{}", Path.string());
            return false;
        };
    }
    return true;
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
    auto len = range.second = range.first;
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
        cursor = pold_chunk->BufCursor + old_range.second - old_range.first + 1;
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

FDownloader::FDownloader(){
    pHttpManager = IHttpManager::GetNamedManager(CURL_HTTP_MANAGER_NAME);
}

DownloadTaskHandle_t FDownloader::AddTask(FDownloadFile* file)
{
    auto respair=Files.try_emplace(DownloadTaskHandle_t::task_count, file);
    if (respair.second) {
        return respair.first->first;
    }
    return DownloadTaskHandle_t();
}

void FDownloader::TransferBuf()
{
    BufList DownloadCompleteBuf;

    for (auto pool_itr = BufPool.begin(); pool_itr != BufPool.end();) {
        auto& pbuf = *pool_itr;
        auto itr = pbuf->ChunkList.begin();
        for (; itr != pbuf->ChunkList.end(); std::advance(itr, 1)) {
            auto& pchunk = *itr;
            auto range=pchunk->GetRange();
            if (pchunk->DownloadSize != range.second-range.first+1) {
                break;
            }
        }
        if (itr == pbuf->ChunkList.end()&& pbuf->ChunkList.size()>0) {
            DownloadCompleteBuf.push_back(pbuf);
            pool_itr=BufPool.erase(pool_itr);
        }
        else {
            std::advance(pool_itr, 1);
        }
    }

    {
        std::scoped_lock lock(BufInIOMtx);
        BufInIO.merge(DownloadCompleteBuf);
    }
    {
        std::scoped_lock lock(BufIOCompleteMtx);
        for (auto& buf:BufIOComplete) {
            buf->ChunkList.clear();
            BufPool.push_back(buf);
        }
        BufIOComplete.clear();
    }
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

FDownloader* FDownloader::Instance() {
    static std::atomic<FDownloader*> atomic_downloader;
    FDownloader*  downloader = atomic_downloader.load();
    if (!downloader) {
        FDownloader* expect{ nullptr };
        if (atomic_downloader.compare_exchange_strong(expect, new FDownloader())) {
            downloader = atomic_downloader.load();
            for (int i = 0; i < BUF_NUM; i++) {
                auto ptr = std::make_shared<FDownloadBuf>(BUF_CHUNK_NUM * FDownloadFile::CHUNK_SIZE);
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
        }
    }
    
    return atomic_downloader.load();
}
FDownloader::~FDownloader()
{
}
//DownloadTaskHandle FDownloader::AddTask(std::string url, std::string path) {
//    FDownloadFile* df = new FDownloadFile(url, path);
//    return AddTask(df);
//}

DownloadTaskHandle_t FDownloader::AddTask(ThroughCRTWrapper<std::string> url, std::string* content)
{
    FDownloadFile* df = new FDownloadFile(url.GetValue(), content);
    return AddTask(df);
}
DownloadTaskHandle_t FDownloader::AddTask(ThroughCRTWrapper<std::string> url, ThroughCRTWrapper<std::filesystem::path> folder)
{
    FDownloadFile* df = new FDownloadFile(url.GetValue(), folder.GetValue().u8string().c_str());
    return AddTask(df);
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

    auto& status = *OutStatus;

    status.DownloadSize = pfile->DownloadSize.load();
    status.PreDownloadSize = pfile->PreDownloadSize.load();
    status.LastTime = pfile->LastTime.load();
    status.PreTime = pfile->PreTime.load();
    status.IsCompelete= pfile->Status == EFileTaskStatus::Finished;
    status.ChunksCompleteFlag = pfile->ChunksCompleteFlag;
    
    return OutStatus;
}

void FDownloader::Tick(float delSec)
{
    auto& HttpManager = *pHttpManager;
    HttpManager.Tick(delSec);
    TransferBuf();
    for (auto itr = RequireRemoveFiles.begin(); itr != RequireRemoveFiles.end(); std::advance(itr,1)) {
        Files.erase(*itr);
    }
    RequireRemoveFiles.clear();
    for (auto& pair : Files) {
        auto pfile = pair.second;
        auto handle = pair.first;
        switch (pfile->Status) {
        case EFileTaskStatus::Idle: {

            for (int i = 0; i < ParallelChunkPerFile; i++) {
                auto preq = HttpManager.NewRequest();
                preq->SetURL(pfile->URL);
                preq->SetVerb("Get");
                pfile->RequestPool.insert(preq);
            }
            auto preq = *pfile->RequestPool.begin();
            preq->SetVerb("HEAD");
            auto pweakFile = pfile->weak_from_this();
            preq->OnProcessRequestComplete() = [pweakFile, handle](HttpRequestPtr req, HttpResponsePtr rep, bool res) {
                {
                    auto pfile = pweakFile.lock();
                    if (!pfile) {
                        return;
                    }
                    if (!res || rep->GetContentLength() <= 0) {
                        pfile->Status = EFileTaskStatus::Finished;
                        pfile->Code = EDownloadCode::SERVER_ERROR;
                        return;
                    }
                    pfile->SetFileSize(rep->GetContentLength());


                    if (!pfile->Content && std::filesystem::is_directory(pfile->Path)) {
                        auto str = std::string(rep->GetHeader("Content-Disposition"));
                        if (str.empty()) {
                            SIMPLELOG_LOGGER_ERROR(nullptr, "FDownloader cant get file name");
                            pfile->Status = EFileTaskStatus::Finished;
                            pfile->Code = EDownloadCode::SERVER_ERROR;
                            return;
                        }
                        try {
                            std::regex filename_reg(
                                R"_(^[^]*filename="([^\/?#\"]+)"[^]*)_",
                                std::regex::ECMAScript
                            );
                            std::smatch match_result;
                            /*url.assign( R"###(localhost.com/path\?hue\=br\#cool)###");*/
                            if (!std::regex_match(str, match_result, filename_reg)) {
                                SIMPLELOG_LOGGER_ERROR(nullptr, "FDownloader cant get file name");
                                pfile->Finish(EDownloadCode::SERVER_ERROR, "FDownloader cant get file name");
                                return;
                            }
                            pfile->Path /= match_result[1].str();
                        }
                        catch (std::exception& ex) {
                            SIMPLELOG_LOGGER_ERROR(nullptr, "{}", ex.what());
                            pfile->Finish(EDownloadCode::SERVER_ERROR, "regex exception");
                            return;
                        }

                    }
                    if (pfile->GetFileInfoDelegate) {
                        auto fileInfo = std::make_shared<DownloadFileInfo_t>();
                        fileInfo->FilePath = pfile->Path;
                        fileInfo->ChunkNum = pfile->ChunkNum;
                        fileInfo->FileSize = pfile->Size;
                        pfile->GetFileInfoDelegate(DownloadTaskHandle(handle), fileInfo);
                    }
                    if (std::filesystem::exists(pfile->Path)) {
                        pfile->Finish(EDownloadCode::OK, "file exist");
                        return;
                    }
                    pfile->Status = EFileTaskStatus::Download;
                    req->SetVerb("GET");
                }
                };
            HttpManager.ProcessRequest(preq);
            pfile->Status = EFileTaskStatus::Init;
            break;
        }                 
        case EFileTaskStatus::Download: {
            while (true) {
                if (pfile->DownloadSize == pfile->Size) {
                    pfile->CloseFileStream();
                    pfile->Status = EFileTaskStatus::Finished;
                }
                if (pfile->IsAllChunkInDownload()) {
                    break;
                }
                auto pchunk = pfile->GetNotDownloadFilechunk();
                if (!pchunk) {
                    SIMPLELOG_LOGGER_ERROR(nullptr, "Downloader GetNotDownloadFilechunk Error");
                    break;
                }

                std::set<HttpRequestPtr>  tempSet;
                std::set<HttpRequestPtr>  RequestSet;
                std::transform(pfile->Requests.cbegin(), pfile->Requests.cend(),
                    std::inserter(RequestSet, RequestSet.begin()),
                    [](const std::pair<HttpRequestPtr, file_chunk_t>& key_value)
                    { return key_value.first; });

                std::set_difference(pfile->RequestPool.cbegin(), pfile->RequestPool.cend(), RequestSet.cbegin(), RequestSet.cend(), std::inserter(tempSet, tempSet.end()));
                if (tempSet.size() == 0) {
                    break;
                }
                auto& req = *tempSet.begin();

                std::shared_ptr<FDownloadBuf> buf;
                buf = InsertInBuf(pchunk);
                if (!buf) {
                    pfile->RevertDownloadFilechunk(pchunk->ChunkIndex);
                    break;
                }
                auto range = pchunk->GetRange();
                req->SetRange(range.first, range.second);
                req->GetResponse()->SetContentBuf(pchunk->BufCursor, pchunk->GetChunkSize());
                std::weak_ptr<file_chunk_t> pweakChunk = pchunk;
                req->OnRequestProgress() = [pweakChunk](HttpRequestPtr req, int64_t oldSize, int64_t newSize, int64_t totalSize) {
                    auto pchunk = pweakChunk.lock();
                    if (!pchunk) {
                        return;
                    }
                    pchunk->DownloadSize = newSize;
                    };
                req->OnProcessRequestComplete() = [pweakChunk, range](HttpRequestPtr req, HttpResponsePtr resp, bool res) {
                    auto pchunk = pweakChunk.lock();
                    if (!pchunk) {
                        return;
                    }
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

            }
            break;
       
        }
        case EFileTaskStatus::Finished: {
            if (pfile->DownloadFinishedDelegate) {
                pfile->DownloadFinishedDelegate(DownloadTaskHandle(pair.first), pfile->Code);
                pfile->DownloadFinishedDelegate = nullptr;
            }
            break;
        }
        }
    }
}

void FDownloader::NetThreadTick(float delSec)
{
    auto& HttpManager = *pHttpManager;
    HttpManager.HttpThreadTick(delSec);
}

void FDownloader::IOThreadTick(float delSec)
{
    BufList IOLocalBufList;
    std::list<std::shared_ptr<file_chunk_t>> CompleteChunk;
    {
        std::scoped_lock lock(BufInIOMtx);
        IOLocalBufList.swap(BufInIO);
    }
    for(auto& buf: IOLocalBufList) {
        for (auto itr = buf->ChunkList.begin(); itr != buf->ChunkList.end();) {
            auto const& chunk = *itr;
            if (chunk->File->SavaDate(chunk) == chunk->GetChunkSize()) {
                CompleteChunk.push_back(chunk);
                itr=buf->ChunkList.erase(itr);
            }
            else {
                std::advance(itr, 1);
            }
        }
    }
    {
        std::scoped_lock lock(BufIOCompleteMtx);
        BufIOComplete.merge(IOLocalBufList);
    }
    {
        for (auto& chunk:CompleteChunk) {
            chunk->File->CompleteChunk(chunk);
        }
    }
    for (auto& chunk : CompleteChunk) {
        auto presize=chunk->File->DownloadSize.fetch_add(chunk->GetChunkSize());
        chunk->File->PreDownloadSize.store(presize);
        auto now = std::chrono::steady_clock::now();
        auto stamp= std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch()).count();
        auto prestamp=chunk->File->LastTime.exchange(stamp);
        chunk->File->PreTime.store(prestamp);
    }
}
