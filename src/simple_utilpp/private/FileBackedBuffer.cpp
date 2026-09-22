#include "FileBackedBuffer.h"

#include "CharBuffer.h"
#include "RawFile.h"
#include "dir_util.h"
#include "simple_error.h"
#include <FunctionExitHelper.h>
#include <moodycamel/concurrentqueue.h>

typedef struct DirtyRange_t {
    void* BeginPos;
    void* EndPos;
}DirtyRange_t;

class FFileBackedBuffer :public IFileBackedBuffer {
public:
    bool Init(uint32_t size, std::u8string_view fileName, std::error_code& ec) override;
    bool Init(std::u8string_view fileName, std::error_code& ec)override;
    void Close() override {
        CloseMap();
        BackupFile.Close();
    }

    bool Clean(std::error_code& ec) override {
        auto view = ConvertViewToU8View(BackupFile.GetFilePath());
        Close();
        auto bres=DirUtil::Delete(view);
        if (!bres) {
            ec= utilpp::make_common_used_error(utilpp::ECommonUsedError::CUE_FILE_OP);
        }
        return bres;
    }
    bool Resize(uint32_t size,std::error_code& ec) {
        assert(BackupFile.IsOpen());
        CloseMap();
        BackupFile.Resize(size,ec);
        if (ec) {
            return false;
        }
        auto bres=OpenMap(ec);
        return bres;
    }

    uint32_t GetSize() {
        if (!BackupFile.IsOpen()) {
            return 0;
        }
        return BackupFile.GetSize();
    }

    void* GetPtr(uint32_t offset) const override {
        return (char*)Data + offset;
    }

    void WriteData(void* target, uint8_t& val)override {
        *(uint8_t*)target = val;
        UpdateDirtyRange(target, (char*)target + sizeof(uint8_t));
    }

    void WriteData(void* target, uint16_t& val)override {
        *(uint16_t*)target = val;
        UpdateDirtyRange(target, (char*)target + sizeof(uint16_t));
    }

    void WriteData(void* target, uint32_t& val) override {
        *(uint32_t*)target = val;
        UpdateDirtyRange(target, (char*)target + sizeof(uint32_t));
    }

    void WriteData(void* target, uint64_t& val) override {
        *(uint64_t*)target = val;
        UpdateDirtyRange(target, (char*)target + sizeof(uint64_t));
    }

    void WriteData(void* target, void* source, uint32_t size) override {
        memcpy(target, source, size);
        UpdateDirtyRange(target, (char*)target + size);
    }

    void UpdateDirtyRange(void* begin, void* end) {
        DirtyRanges.enqueue(DirtyRange_t{ begin,end });
    }

    void TickIO(float delSec) override;
    void* Data{};
    FRawFile BackupFile;
    moodycamel::ConcurrentQueue<DirtyRange_t> DirtyRanges;
    std::vector<DirtyRange_t> DirtyRangeCache;
#ifdef _WIN32
    F_HANDLE hmap{};
#endif

private:
    void CloseMap() {
#ifdef _WIN32
        if (Data) {
            UnmapViewOfFile(Data);
            Data = NULL;
        }
        if (hmap) {
            CloseHandle(hmap);
            hmap = NULL;
        }
#else
        if (Data) {
            munmap(Data, BackupFile.Size());
            Data = NULL;
        }
#endif
    }
    bool OpenMap(std::error_code & ec) {
#ifdef _WIN32
        hmap = CreateFileMappingA(
            BackupFile.GetHandle(),
            NULL,
            PAGE_READWRITE,
            0,
            BackupFile.GetSize(),
            NULL
        );
        if (!hmap) {
            auto ires=GetLastError();
            ec = std::make_error_code(std::errc::io_error);
            return false;
        }

        Data = MapViewOfFile(hmap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (!Data) {
            ec = std::make_error_code(std::errc::io_error);
            return false;
        }
#else
        Data = mmap(
            nullptr,
            BackupFile.GetSize(),
            PROT_READ | PROT_WRITE,
            MAP_SHARED, // MAP_SHARED 确保修改会写回文件
            BackupFile.GetHandle(),
            0
        );
        if (Data == MAP_FAILED) {
            ec = std::make_error_code(std::errc::io_error);
            return false;
        }
#endif
        return true;
    }
};



bool FFileBackedBuffer::Init(uint32_t size, std::u8string_view fileName, std::error_code& ec)
{
    bool bres{ true };
    bres = BackupFile.Open(fileName, UTIL_OPEN_ALWAYS, 0, ec);
    if (!bres) {
        return bres;
    }

    if (ec == std::make_error_code(std::errc::file_exists)) {
        ec.clear();
        if (BackupFile.GetSize() != size&& size != std::numeric_limits<uint32_t>::max()) {
            BackupFile.Resize(size, ec);
            if (ec) {
                return false;
            }
            ec = std::make_error_code(std::errc::bad_message);
        }
        else {
            ec = std::make_error_code(std::errc::file_exists);
        }
    }
    else {
        ec.clear();
        if (size != std::numeric_limits<uint32_t>::max()) {
            BackupFile.Resize(size, ec);
            if (ec) {
                return false;
            }
        }
        //memset(Buf.Data(), 0, Buf.Size());
        //BackupFile.Write(Buf.Data(), Buf.Size());
    }
    if (BackupFile.GetSize() != 0) {
        bres = OpenMap(ec);
    }
    return bres;
}

bool FFileBackedBuffer::Init(std::u8string_view fileName, std::error_code& ec)
{
    return Init(std::numeric_limits<uint32_t>::max(), fileName, ec);
}

void FFileBackedBuffer::TickIO(float delSec)
{
    std::array<DirtyRange_t, 10> tmp;
    while (true) {
        auto outSize = DirtyRanges.try_dequeue_bulk(tmp.data(), tmp.size());
        DirtyRangeCache.insert(DirtyRangeCache.end(), tmp.begin(), tmp.begin() + outSize);
        if (outSize < tmp.size()) {
            break;
        }
    }

    if (DirtyRangeCache.size() == 0) {
        return;
    }
    std::sort(DirtyRangeCache.begin(), DirtyRangeCache.end(),
        [](const DirtyRange_t& a, const DirtyRange_t& b) {
            return reinterpret_cast<uintptr_t>(a.BeginPos) <
                reinterpret_cast<uintptr_t>(b.BeginPos);
        }
    );

    size_t w = 0;
    for (size_t i = 1; i < DirtyRangeCache.size(); ++i) {
        if (DirtyRangeCache[i].BeginPos <= DirtyRangeCache[w].EndPos) {
            DirtyRangeCache[w].EndPos = std::max(DirtyRangeCache[w].EndPos, DirtyRangeCache[i].EndPos);
        }
        else {
            DirtyRangeCache[++w] = DirtyRangeCache[i];
        }
    }
    DirtyRangeCache.resize(w + 1);
    for (auto& range : DirtyRangeCache) {
#ifdef _WIN32
        FlushViewOfFile(range.BeginPos, (char*)range.EndPos - (char*)range.BeginPos);
#else
        msync(range.BeginPos, (char*)range.EndPos - (char*)range.BeginPos, MS_SYNC);
#endif
    }
    BackupFile.Flush();
    DirtyRangeCache.clear();
}

IFileBackedBuffer* NewFileBackedBuffer()
{
    return new FFileBackedBuffer;
}

void FreeFileBackedBuffer(IFileBackedBuffer* ptr)
{
    return delete ptr;
}