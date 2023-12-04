#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <curl\curl.h>


//typedef void(*download_done_cb)(int err, void *userp);
typedef void(*download_progress_cb)(int64_t total, int64_t now, void* userp);

class FDownloadFile {
private:
    struct buf_s;
public:
    typedef struct file_chunk_s {
        uint32_t begin;
        int length;
        uint32_t dl_size;
        FDownloadFile* file;
        buf_s* buf;
        uint32_t getRangeBegin() {
            return buf->begin + begin;
        }
        char* getChunkBuf() { return buf->buf + begin; }
    }file_chunk_t;
    FDownloadFile(std::string url, std::string folder);
    FDownloadFile(std::string url, std::string* content);
    ~FDownloadFile();
    file_chunk_t* GetFilechunk();
    bool HasChunk();
    bool IsIntact();
    bool Init();
    std::string Geturl();
    size_t SavaDate(file_chunk_t* file_chunk, int length, char* data);
    void CompleteChunk(file_chunk_t* file_chunk);
    int32_t GetLength();
    uint32_t GetDlLength();

    FDownloadFile& operator=(const FDownloadFile& other) = delete;
    FDownloadFile(FDownloadFile&) = delete;
    FDownloadFile& operator=(const FDownloadFile&& other) = delete;
    FDownloadFile(FDownloadFile&&) = delete;
private:
    typedef struct buf_s {
        char* buf;
        uint32_t begin;
        int length;
        uint32_t dl_size;
        buf_s(int begin, int length, int dl_size = 0) {
            this->begin = begin;
            this->length = length;
            this->dl_size = dl_size;
            buf = (char*)malloc(CHUNK_SIZE * CHUNK_NUM);
        }
        ~buf_s() {
            free(buf);
        }
    }buf_t;
    static int CHUNK_SIZE;
    static int CHUNK_NUM;
    bool intact;
    FILE* fp;
    int64_t size;
    uint32_t dl_size;
    uint32_t buf_size;
    std::string path;
    std::string filename;
    std::string url;
    std::string* content;
    std::mutex mtx;
    typedef std::list<file_chunk_t*> chunk_list;
    typedef std::pair<buf_t*, chunk_list> buf_element;
    typedef std::list<buf_element>  buf_list;
    buf_list  bufs;

    bool AddBuf();
    bool Open();
    void GetContentLength();
};


class FDownloader
{
public:
    enum DMcode {
        DM_OK,
        DM_INTERNAL_ERROR,
        DM_FINISHED,
        DM_COULDNT_CONNECT,
        DM_TIMEOUT,
        DM_SSL_CONNECT_ERROR,
        DM_SERVER_ERROR
    };
    typedef void(*download_done_cb)(DMcode code, int http_code, void* userp);
    typedef void(*download_progress_cb)(int64_t total, int64_t now, void* userp);

    static FDownloader& Instance();
    ~FDownloader();
    DMcode AddTask(std::string url, std::string folder);
    DMcode AddTask(std::string url, std::string* content);
    DMcode AddTask(std::string url, std::string* content, download_done_cb done_cb, void* userp = nullptr);
    DMcode Perform();
    void SetProgressCB(download_progress_cb progress_cb, void* userp = nullptr);
    void SetOvertime(uint32_t seconde);


    FDownloader& operator=(const FDownloadFile& other) = delete;
    FDownloader(FDownloadFile&) = delete;
    FDownloader& operator=(const FDownloadFile&& other) = delete;
    FDownloader(FDownloadFile&&) = delete;

protected:
private:
    FDownloader();
    DMcode Init();
    void Clear();
    bool CheckTimeout();
    DMcode AddFilechunk();

    static size_t dl_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
    enum DState {
        INVALID,
        READY,
        RUNNING
    };
    typedef struct file_ex_data_s {
        download_progress_cb progress_cb;
        download_done_cb done_cb;
        void* userp;
        file_ex_data_s() :progress_cb(nullptr), done_cb(nullptr), userp(nullptr) {
        }
    }file_ex_data_t;
    typedef struct curl_writedata_s {
        FDownloadFile::file_chunk_t* file_chunk;
        download_progress_cb progress_cb;
        uint32_t* dl_size_sum;
        int* size_sum;
        void* userp;
        std::chrono::time_point<std::chrono::steady_clock>* last_update;
        std::unordered_map<FDownloadFile*, file_ex_data_t*>* cfiles_ex;
    }curl_writedata_t;

    download_progress_cb progress_cb;
    void* userp;
    CURLM* hcurlm;
    int thread_num;
    DState state;
    uint32_t dl_size_sum;
    int size_sum;
    int file_sel_num;
    uint32_t overtime;
    std::chrono::time_point<std::chrono::steady_clock> last_update;
    typedef std::unordered_map<CURL*, FDownloadFile::file_chunk_t*> task_map;
    task_map tasks;
    typedef std::unordered_map<FDownloadFile*, file_ex_data_t*> files_ex_map;
    files_ex_map cfiles_ex;
    std::vector<FDownloadFile*> cfiles;
    std::vector<FDownloadFile*> ccomplete_files;
};