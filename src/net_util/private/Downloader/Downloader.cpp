#include"Downloader/downloader.h"
#include <chrono>
#include <thread>
#include <string>
#include <filesystem>

const int MAX_WRITE_SIZE = 16384;
int FDownloadFile::CHUNK_SIZE = MAX_WRITE_SIZE * 16;
int FDownloadFile::CHUNK_NUM = 4;

FILE* fopen_cwd(const char* path, const char* mode)
{
    std::filesystem::path p(path);
    if (!p.is_absolute()) {
        p = std::filesystem::current_path() / path;
        p=p.lexically_normal();
    }

    if (!std::filesystem::exists(p.parent_path())) {
        std::filesystem::create_directories(p.parent_path());
    }
    return fopen((char*)p.u8string().c_str(), mode);
}

FDownloadFile::FDownloadFile(std::string url, std::string path) :size(-1), buf_size(0), dl_size(0), url(url), path(path), content(nullptr), fp(nullptr) {}
FDownloadFile::FDownloadFile(std::string url, std::string* content) : size(-1), buf_size(0), dl_size(0), url(url), content(content), fp(nullptr) {}
FDownloadFile::~FDownloadFile()
{
    this;
    if (fp != nullptr) {
        fclose(fp);
    }
}

FDownloadFile::file_chunk_t* FDownloadFile::GetFilechunk()
{
    file_chunk_t* file_chunk = nullptr;
    for (auto& buf : bufs) {
        const auto chunk = buf.second.begin();
        if (chunk != buf.second.end()) {
            file_chunk = *chunk;
            buf.second.erase(chunk);
            return file_chunk;
        }
    }

    if (AddBuf()) {
        auto& buf = *--bufs.end();
        const auto chunk = buf.second.begin();
        if (chunk != buf.second.end()) {
            file_chunk = *chunk;
            buf.second.erase(chunk);
            return file_chunk;
        }
    }
    return file_chunk;
}

bool FDownloadFile::HasChunk()
{
    if (size == -1) {
        return bufs.size() < 1;
    }
    if (size == buf_size) {
        for (auto& buf : bufs) {
            const auto chunk = buf.second.begin();
            if (chunk != buf.second.end()) {
                return true;
            }
        }
    }
    else {
        return true;
    }
    return false;
}

bool FDownloadFile::IsIntact()
{
    return size == dl_size;
}

bool FDownloadFile::Init()
{
    GetContentLength();
    if (content == nullptr && !Open()) {
        return false;
    }
    return true;
}

std::string FDownloadFile::Geturl()
{
    printf("+++++++++++++url:[%s]\n", url.c_str());
    return url;
}

size_t FDownloadFile::SavaDate(file_chunk_t* file_chunk, int length, char* data)
{
    buf_t* buf = file_chunk->buf;
    dl_size += length;
    if (file_chunk->length == -1) {
        if (length + buf->dl_size > CHUNK_SIZE * CHUNK_NUM) {
            memcpy(buf->buf + buf->dl_size, data, CHUNK_SIZE * CHUNK_NUM - buf->dl_size);
            if (content) {
                content->append(buf->buf, CHUNK_SIZE * CHUNK_NUM);
            }
            else {
                fwrite(buf->buf, CHUNK_SIZE * CHUNK_NUM, 1, fp);
            }
            buf->dl_size = length + buf->dl_size - CHUNK_SIZE * CHUNK_NUM;
            buf->begin += CHUNK_SIZE * CHUNK_NUM;
            memcpy(buf->buf, data, buf->dl_size);
        }
        else {
            memcpy(buf->buf + buf->dl_size, data, length);
            buf->dl_size += length;
        }
    }
    else {
        memcpy(file_chunk->getChunkBuf() + file_chunk->dl_size, data, length);
        file_chunk->dl_size += length;
        buf->dl_size += length;
        if (file_chunk->dl_size == file_chunk->length) {
            mtx.lock();
            for (auto itr = bufs.begin(); itr != bufs.end() && (itr->first->dl_size == itr->first->length);) {
                if (content) {
                    content->append(itr->first->buf, itr->first->length);
                }
                else {
                    fwrite(itr->first->buf, itr->first->length, 1, fp);
                }
                itr = bufs.erase(itr);
            }
            mtx.unlock();
        }
    }
    return length;
}

void FDownloadFile::CompleteChunk(file_chunk_t* file_chunk)
{
    if (size == -1) {
        if (content) {
            content->append(file_chunk->buf->buf, file_chunk->buf->dl_size);
        }
        else {
            fwrite(file_chunk->buf->buf, file_chunk->buf->dl_size, 1, fp);
        }
    }
}

int32_t FDownloadFile::GetLength()
{
    return size;
}

uint32_t FDownloadFile::GetDlLength()
{
    return dl_size;
}

bool FDownloadFile::AddBuf()
{
    if (size == -1 && !bufs.size()) {
        chunk_list list;
        buf_t* buf = new buf_t(buf_size, 0);
        file_chunk_t* file_chunk = new file_chunk_t();
        file_chunk->begin = 0;
        file_chunk->buf = buf;
        file_chunk->dl_size = 0;
        file_chunk->file = this;
        file_chunk->length = 0;
        list.push_back(file_chunk);
        bufs.push_back(buf_element(buf, list));
        return true;
    }
    else if (buf_size < size) {
        chunk_list list;
        buf_t* buf = new buf_t(buf_size, CHUNK_SIZE * CHUNK_NUM > (size - buf_size) ? (size - buf_size) : CHUNK_SIZE * CHUNK_NUM);
        buf_size += buf->length;
        for (int buf_len = 0; buf_len < buf->length; ) {
            file_chunk_t* file_chunk = new file_chunk_t();
            file_chunk->begin = buf_len;
            file_chunk->buf = buf;
            file_chunk->dl_size = 0;
            file_chunk->file = this;
            file_chunk->length = CHUNK_SIZE > (buf->length - buf_len) ? buf->length - buf_len : CHUNK_SIZE;
            list.push_back(file_chunk);
            buf_len += file_chunk->length;
        }
        bufs.push_back(buf_element(buf, list));
        return true;
    }
    return false;
}

bool FDownloadFile::Open()
{
    if (size > 0) {
        fp = fopen_cwd(path.c_str(), "r");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            buf_size = dl_size = ftell(fp);
            fclose(fp);
            if (dl_size >= size) {
                goto redownload;
            }
        }
        fp = fopen_cwd(path.c_str(), "ab+");
    }
    else {
    redownload:
        buf_size = dl_size = 0;
        fp = fopen_cwd(path.c_str(), "wb+");
    }
    if (fp != nullptr)
        return true;
    else
        return false;
}

void FDownloadFile::GetContentLength()
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        return;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

    CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        return;
    }
    curl_off_t cl;
    code = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
    if (code != CURLE_OK) {
        return;
    }
    size = static_cast<int64_t>(cl);
    curl_easy_cleanup(curl);
}

FDownloader::FDownloader() : thread_num(5), file_sel_num(0), progress_cb(nullptr), userp(nullptr),
dl_size_sum(0), size_sum(0), overtime(0) {
    CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLcode::CURLE_OK) {
        state = INVALID;
    }
    else {
        state = READY;
    }
}
FDownloader::DMcode FDownloader::Init()
{
    hcurlm = curl_multi_init();
    if (hcurlm == nullptr) {
        return  DM_INTERNAL_ERROR;
    }
    return DM_OK;
}
void FDownloader::Clear()
{
    for (auto task : tasks) {
        CURL* curl = task.first;
        curl_multi_remove_handle(hcurlm, curl);
        curl_easy_cleanup(curl);
        delete task.second;
    }
    tasks.clear();
    curl_multi_cleanup(hcurlm);

    while (ccomplete_files.size()) {
        delete* --ccomplete_files.end();
        ccomplete_files.pop_back();
    }
    cfiles_ex.clear();
    dl_size_sum = 0;
    size_sum = 0;
    userp = nullptr;
}
bool FDownloader::CheckTimeout()
{
    if (!overtime) {
        return false;
    }
    std::chrono::seconds timeout(overtime);
    if (std::chrono::steady_clock::now() - last_update > timeout) {
        return  true;
    }
    else {
        return false;
    }
}
FDownloader::DMcode FDownloader::AddFilechunk()
{
    if (cfiles.size() == 0) {
        return DM_FINISHED;
    }
    while (tasks.size() < thread_num) {
        if (cfiles.size() == 0) {
            break;
        }
        if (file_sel_num >= cfiles.size()) {
            file_sel_num = 0;
        }
        FDownloadFile::file_chunk_t* file_chunk = cfiles[file_sel_num]->GetFilechunk();
        if (!cfiles[file_sel_num]->HasChunk()) {
            ccomplete_files.push_back(*(cfiles.begin() + file_sel_num));
            cfiles.erase(cfiles.begin() + file_sel_num);
        }
        CURL* curl = curl_easy_init();
        if (curl == nullptr) {
            return DM_INTERNAL_ERROR;
        }
        char buf[BUFSIZ];
        if (file_chunk->length) {
            sprintf_s(buf, "%d-%d", file_chunk->getRangeBegin(), file_chunk->getRangeBegin() + file_chunk->length - 1);
            curl_easy_setopt(curl, CURLOPT_RANGE, buf);
        }
        curl_easy_setopt(curl, CURLOPT_URL, file_chunk->file->Geturl().c_str());

        //curl_easy_setopt(dlark->easy_handles[index], CURLOPT_WRITEDATA, dlark->fss[index].fp);
        curl_writedata_t* writedata = new curl_writedata_t;
        writedata->file_chunk = file_chunk;
        writedata->progress_cb = progress_cb;
        writedata->dl_size_sum = &dl_size_sum;
        writedata->size_sum = &size_sum;
        writedata->cfiles_ex = &cfiles_ex;
        writedata->userp = userp;
        writedata->last_update = &last_update;
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, writedata);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dl_write_callback);
        //curl_easy_setopt(curl, CURLOPT_PRIVATE, file_chunk);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        //curl_easy_setopt(dlark->easy_handles[index], CURLOPT_XFERINFOFUNCTION, download_files_progress);
        //curl_easy_setopt(dlark->easy_handles[index], CURLOPT_XFERINFODATA, &dlark->darks[index]);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
        if (curl_multi_add_handle(hcurlm, curl) != CURLM_OK) {
            return DM_INTERNAL_ERROR;
        }
        ++file_sel_num;
        tasks.emplace(curl, file_chunk);
    }
    return DM_OK;
}
size_t FDownloader::dl_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    curl_writedata_t* writedata = static_cast<curl_writedata_t*>(userdata);
    FDownloadFile::file_chunk_t* file_chunk = writedata->file_chunk;
    download_progress_cb progress_cb = writedata->progress_cb;
    FDownloadFile* file = file_chunk->file;
    uint32_t* dl_size_sum = writedata->dl_size_sum;
    int* size_sum = writedata->size_sum;
    int dl_len = size * nmemb;
    *writedata->last_update = std::chrono::steady_clock::now();
    size_t res = file->SavaDate(file_chunk, dl_len, ptr);
    *dl_size_sum += dl_len;
    if (progress_cb) {
        progress_cb(*size_sum, *dl_size_sum, writedata->userp);
    }

    files_ex_map* map = writedata->cfiles_ex;
    auto file_ex = map->find(file);
    if (file_ex != map->end()) {
        if (file->GetLength() == file->GetDlLength() && file_ex->second->done_cb) {
            file_ex->second->done_cb(DM_OK, 200, file_ex->second->userp);
        }
    }

    return res;
}

FDownloader& FDownloader::instance() {
    static FDownloader* downloader;
    if (!downloader) {
        downloader = new FDownloader();
    }
    return *downloader;
}
FDownloader::~FDownloader()
{
    curl_global_cleanup();

}
FDownloader::DMcode FDownloader::AddTask(std::string url, std::string path) {
    FDownloadFile* df = new FDownloadFile(url, path);

    if (!df->Init()) {
        return DM_INTERNAL_ERROR;
    }
    cfiles.push_back(df);
    if (size_sum >= 0) {
        if (df->GetLength() >= 0) {
            size_sum += df->GetLength();
            dl_size_sum += df->GetDlLength();
        }
        else {
            size_sum = -1;
            dl_size_sum += df->GetDlLength();
        }
    }

    return DM_OK;
}
FDownloader::DMcode FDownloader::AddTask(std::string url, std::string* content) {
    FDownloadFile* df = new FDownloadFile(url, content);

    if (!df->Init()) {
        return DM_INTERNAL_ERROR;
    }
    cfiles.push_back(df);
    if (size_sum >= 0) {
        if (df->GetLength() >= 0) {
            size_sum += df->GetLength();
            dl_size_sum += df->GetDlLength();
        }
        else {
            size_sum = -1;
            dl_size_sum += df->GetDlLength();
        }
    }

    return DM_OK;
}
FDownloader::DMcode FDownloader::AddTask(std::string url, std::string* content, download_done_cb _done_cb, void* _userp)
{
    FDownloadFile* df = new FDownloadFile(url, content);

    if (!df->Init()) {
        return DM_INTERNAL_ERROR;
    }
    cfiles.push_back(df);
    file_ex_data_t* data = new file_ex_data_t;
    data->done_cb = _done_cb;
    data->userp = _userp;
    cfiles_ex.insert(files_ex_map::value_type(df, data));
    if (size_sum >= 0) {
        if (df->GetLength() >= 0) {
            size_sum += df->GetLength();
            dl_size_sum += df->GetDlLength();
        }
        else {
            size_sum = -1;
            dl_size_sum += df->GetDlLength();
        }
    }

    return DM_OK;
}
FDownloader::DMcode FDownloader::Perform()
{
    Init();
    int still_running = 0; /* keep number of running handles */
    int repeats = 0;
    int numfds = 0;
    CURLMsg* curlmsgp = nullptr;
    CURLMsg curlmsg;
    DMcode res = DM_OK;
    int pending_messages = 0;
    AddFilechunk();
    CURLMcode mcode = curl_multi_perform(hcurlm, &still_running);
    if (mcode != CURLM_OK) {
        return DM_INTERNAL_ERROR;
    }
    if (still_running == 0) {
        return res;
    }
    while (true) {
        mcode = curl_multi_wait(hcurlm, nullptr, 0, 1000, &numfds);
        if (CheckTimeout()) {
            return DM_TIMEOUT;
        };
        if (mcode != CURLM_OK) {
            return DM_INTERNAL_ERROR;
        }
        if (!numfds) {
            if (++repeats > 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        else {
            repeats = 0;
        }
        mcode = curl_multi_perform(hcurlm, &still_running);
        if (mcode != CURLM_OK) {
            return DM_INTERNAL_ERROR;
        }
        while ((curlmsgp = curl_multi_info_read(hcurlm, &pending_messages)) != nullptr) {
            // @see: https://curl.haxx.se/libcurl/c/curl_multi_info_read.html
            memcpy(&curlmsg, curlmsgp, sizeof(CURLMsg));
            //spdlog::get("update")->info("multi_info_read:{}", curlmsg.msg);
            if (curlmsg.msg == CURLMSG_DONE) {
                CURL* curl = curlmsg.easy_handle;
                task_map::iterator itr = tasks.find(curl);
                FDownloadFile* file = itr->second->file;
                long http_code = -1;
                switch (curlmsg.data.result) {
                case CURLE_OK: {
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                    if (http_code < 200 || http_code >= 300) {
                        res = DM_SERVER_ERROR;
                    }
                    break;
                }
                case CURLE_COULDNT_CONNECT:
                    res = DM_COULDNT_CONNECT;
                    break;
                case CURLE_SSL_CONNECT_ERROR:
                    res = DM_SSL_CONNECT_ERROR;
                    break;
                default:
                    res = DM_INTERNAL_ERROR;
                    break;
                }
                if (file->GetLength() == -1) {
                    file->CompleteChunk(itr->second);
                    auto file_ex = cfiles_ex.find(file);
                    if (file_ex != cfiles_ex.end() && file_ex->second->done_cb) {
                        auto data = file_ex->second;
                        data->done_cb(res, http_code, data->userp);
                    }
                }
                tasks.erase(itr);
                curl_multi_remove_handle(hcurlm, curl);
                curl_easy_cleanup(curl);
            }
        }
        if (AddFilechunk() == DM_FINISHED && still_running == 0) {
            break;
        }
    }
    Clear();
    return res;
}
void FDownloader::SetProgressCB(FDownloader::download_progress_cb _progress_cb, void* _userp)
{
    this->progress_cb = _progress_cb;
    this->userp = _userp;
}

void FDownloader::SetOvertime(uint32_t seconde)
{
    overtime = seconde;
}