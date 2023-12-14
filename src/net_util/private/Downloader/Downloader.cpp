#include"Downloader/downloader.h"
#include <chrono>
#include <logger.h>
#include <thread>
#include <regex>

const uint32_t FDownloadFile::CHUNK_SIZE = 4*1024*1024;
const uint32_t FDownloader::BUF_NUM = 4;
const uint32_t FDownloader::BUF_CHUNK_NUM = 3;



FDownloadFile::FDownloadFile(std::string url, std::string path) : Path(path), URL(url){}
FDownloadFile::FDownloadFile(std::string url, std::string* content) : Content(content), URL(url) {}
FDownloadFile::FDownloadFile(std::string url, std::filesystem::path folder) : Path(folder), URL(url) {}
FDownloadFile::FDownloadFile() {}
FDownloadFile::~FDownloadFile()
{
}

void FDownloadFile::SetFileSize(uint64_t size)
{
    Size = size;
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
        LOG_ERROR("GetNotDownloadFilechunk without shared_ptr");
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
        if (FileStream.Open((const char*)Path.u8string().c_str(), UTIL_OPEN_ALWAYS, Size)!= ERR_SUCCESS) {
            LOG_ERROR("open file failed path:{}", Path.string());
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
        LOG_ERROR("InsertInBuf without shared_ptr");
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
}

DownloadTaskHandle FDownloader::AddTask(FDownloadFile* file)
{
    auto lock=std::unique_lock(FilesMtx);
    auto respair=Files.emplace(DownloadTaskHandle::task_count, file);
    lock.unlock();
    if (respair.second) {
        return respair.first->first;
    }
    return DownloadTaskHandle();
}

void FDownloader::TransferBuf()
{
    BufList DownloadCompleteBuf;
    {
        std::scoped_lock(BufPoolMtx);
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
            if (itr == pbuf->ChunkList.end()) {
                DownloadCompleteBuf.push_back(pbuf);
                BufPool.erase(pool_itr);
            }
            else {
                std::advance(pool_itr, 1);
            }
        }
    }
    {
        std::scoped_lock(BufInIOMtx);
        BufInIO.merge(DownloadCompleteBuf);
    }
    {
        std::scoped_lock(BufIOCompleteMtx, BufPoolMtx);
        for (auto& buf:BufIOComplete) {
            buf->ChunkList.clear();
            BufPool.push_back(buf);
        }
        BufIOComplete.clear();
    }
}

std::shared_ptr<FDownloadBuf> FDownloader::InsertInBuf(std::shared_ptr < file_chunk_t> chunk)
{
    std::scoped_lock(BufPoolMtx);
    for (auto& pbuf : BufPool) {
        auto res=pbuf->InsertInBuf(chunk);
        if (res) {
            return pbuf;
        }
    }
    return nullptr;
}

FDownloader* FDownloader::Instance() {
    static FDownloader* downloader;
    if (!downloader) {
        downloader = new FDownloader();
        if (downloader) {
            for (int i = 0; i < BUF_NUM; i++) {
                downloader->BufPool.push_back(std::make_shared<FDownloadBuf>(BUF_CHUNK_NUM * FDownloadFile::CHUNK_SIZE));
                if (!downloader->BufPool.back()->Buf) {
                    delete downloader;
                }
            }
        }
    }
    
    return downloader;
}
FDownloader::~FDownloader()
{
}
DownloadTaskHandle FDownloader::AddTask(std::string url, std::string path) {
    FDownloadFile* df = new FDownloadFile(url, path);
    return AddTask(df);
}

DownloadTaskHandle FDownloader::AddTask(std::string url, std::string* content)
{
    FDownloadFile* df = new FDownloadFile(url, content);
    return AddTask(df);
}
DownloadTaskHandle FDownloader::AddTask(std::string url, std::filesystem::path folder)
{
    FDownloadFile* df = new FDownloadFile(url, folder.u8string().c_str());
    return AddTask(df);
}

void FDownloader::Tick()
{
    HttpManager.Tick();
    TransferBuf();
    std::scoped_lock(FilesMtx);
    for (auto& pair : Files) {
        auto pfile = pair.second;
        switch (pfile->Status) {
        case EFileTaskStatus::Idle: {

            for (int i = 0; i < ParallelChunkPerFile;i++) {
                auto preq = HttpManager.NewRequest();
                preq->SetURL(pfile->URL);
                preq->SetVerb("Get");
                pfile->RequestPool.insert(preq);
            }
            auto preq = *pfile->RequestPool.begin();
            preq->SetVerb("HEAD");
            preq->OnProcessRequestComplete() = [=](HttpRequestPtr req, HttpResponsePtr rep, bool res) {
                {
                    if (!res|| req->GetContentLength() <= 0) {
                        std::scoped_lock(FilesMtx);
                        pfile->Status = EFileTaskStatus::Finished;
                        pfile->Code = EDownloadCode::SERVER_ERROR;
                        return;
                    }
                    pfile->SetFileSize(req->GetContentLength());


                    if (!pfile->Content && std::filesystem::is_directory(pfile->Path)) {
                        auto str = rep->GetHeader("Content-Disposition");
                        if (str.empty()) {
                            LOG_ERROR("FDownloader cant get file name");
                            std::scoped_lock(FilesMtx);
                            pfile->Status = EFileTaskStatus::Finished;
                            pfile->Code = EDownloadCode::SERVER_ERROR;
                            return;
                        }
                        std::regex filename_reg(
                            R"_(^[^]*filename="([^\/?#\"]+)"[^]*)_",
                            std::regex::extended
                        );
                        std::smatch match_result;
                        /*url.assign( R"###(localhost.com/path\?hue\=br\#cool)###");*/
                        if (!std::regex_match(str, match_result, filename_reg)) {
                            LOG_ERROR("FDownloader cant get file name");
                            std::scoped_lock(FilesMtx);
                            pfile->Status = EFileTaskStatus::Finished;
                            pfile->Code = EDownloadCode::SERVER_ERROR;
                            return;
                        }
                        std::scoped_lock(FilesMtx);
                        pfile->Path /= match_result[1].str();
                        pfile->Status = EFileTaskStatus::Download;
                    }
                    req->SetVerb("GET");
                }
            };
            HttpManager.ProcessRequest(preq);
            pfile->Status = EFileTaskStatus::Init;
            break;
        }

        case EFileTaskStatus::Download: {
            while (true) {
                if (pfile->IsAllChunkInDownload()) {
                    break;
                }
                auto pchunk = pfile->GetNotDownloadFilechunk();
                if (!pchunk) {
                    LOG_ERROR("Downloader GetNotDownloadFilechunk Error");
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
                auto& req =  *tempSet.begin();

                std::shared_ptr<FDownloadBuf> buf;
                buf = InsertInBuf(pchunk);
                if (!buf) {
                    break;
                }
                auto range = pchunk->GetRange();
                req->SetRange(range.first, range.second);
                req->SetContentBuf(pchunk->BufCursor, pchunk->GetChunkSize());
                req->OnRequestProgress() = [&, pchunk](HttpRequestPtr req, int64_t oldSize, int64_t newSize,  int64_t totalSize) {
                    pchunk->DownloadSize = newSize;
                    };
                req->OnProcessRequestComplete() = [&, pchunk, range](HttpRequestPtr req, HttpResponsePtr resp, bool res) {
                    if (!res) {
                        pchunk->File->RevertDownloadFilechunk(pchunk->ChunkIndex);
                        pchunk->Buf->RemoveChunk(pchunk);
                        return;
                    }
                    if (resp->GetContent().size() != pchunk->GetChunkSize()) {
                        pchunk->File->RevertDownloadFilechunk(pchunk->ChunkIndex);
                        pchunk->Buf->RemoveChunk(pchunk);
                        LOG_ERROR("Downloaded size not equal range");
                        return;
                    }
                    req->OnRequestProgress() = nullptr;
                    pchunk->DownloadSize = resp->GetContent().size();
                    };
                HttpManager.ProcessRequest(req);
                
            }
            
            break;
        }
        }
    }
}

void FDownloader::NetThreadTick()
{
    HttpManager.HttpThreadTick();
}

void FDownloader::IOThreadTick()
{
    BufList IOLocalBufList;
    std::list<std::shared_ptr<file_chunk_t>> CompleteChunk;
    {
        std::scoped_lock(BufInIOMtx);
        IOLocalBufList.swap(BufInIO);
    }
    for(auto& buf: IOLocalBufList) {
        for (auto itr = buf->ChunkList.begin(); itr != buf->ChunkList.end();std::advance(itr,1)) {
            auto const& chunk = *itr;
            if (chunk->File->SavaDate(chunk) == chunk->GetChunkSize()) {
                buf->ChunkList.erase(itr);
                CompleteChunk.push_back(chunk);
            }
        }
    }
    {
        std::scoped_lock(BufIOCompleteMtx);
        BufIOComplete.swap(IOLocalBufList);
    }
    {
        std::scoped_lock(FilesMtx);
        for (auto& chunk:CompleteChunk) {
            chunk->File->CompleteChunk(chunk);
        }
    }
}
