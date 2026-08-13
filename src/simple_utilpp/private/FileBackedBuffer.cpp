#include "FileBackedBuffer.h"

#include "CharBuffer.h"
#include "RawFile.h"
#include "dir_util.h"
#include "simple_error.h"
#include <moodycamel/concurrentqueue.h>

typedef struct DirtyRange_t {
    void* BeginPos;
    void* EndPos;
}DirtyRange_t;

class FFileBackedBuffer :public IFileBackedBuffer {
public:
    bool Init(uint32_t size, std::u8string_view fileName, std::error_code& ec) override {
        Buf.Resize(size);
        auto bres = BackupFile.Open(fileName, UTIL_OPEN_ALWAYS, 0, ec);
        if (!bres) {
            return false;
        }

        if (ec == std::make_error_code(std::errc::file_exists)) {
            ec.clear();
            auto readed = BackupFile.Read(Buf.Data(), size, ec);
            if (ec) {
                return false;
            }
            if (readed != size) {
                memset(Buf.Data(), 0, Buf.Size());
                ec = std::make_error_code(std::errc::bad_message);
                return true;
            }
            else {
                ec = std::make_error_code(std::errc::file_exists);
                return true;
            }
        }
        else {
            ec.clear();
            memset(Buf.Data(), 0, Buf.Size());
            BackupFile.Write(Buf.Data(), Buf.Size());
            return true;
        }
        return true;
    }

    void Close() override {
        BackupFile.Close();
    }

    bool Clean(std::error_code& ec) override {
        Buf.Resize(0);
        auto view = ConvertViewToU8View(BackupFile.GetFilePath());
        Close();
        auto bres=DirUtil::Delete(view);
        if (!bres) {
            ec= utilpp::make_common_used_error(utilpp::ECommonUsedError::CUE_FILE_OP);
        }
        return bres;
    }

    void* GetPtr(uint32_t offset) const override {
        return Buf.Data() + offset;
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

    void IOTick(float delSec) override {
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
            BackupFile.Seek((char*)range.BeginPos - Buf.Data());
            BackupFile.Write(range.BeginPos, (char*)range.EndPos - (char*)range.BeginPos);
        }
        DirtyRangeCache.clear();
    }
    FCharBuffer Buf;
    FRawFile BackupFile;
    moodycamel::ConcurrentQueue<DirtyRange_t> DirtyRanges;
    std::vector<DirtyRange_t> DirtyRangeCache;
};

IFileBackedBuffer* NewFileBackedBuffer()
{
    return new FFileBackedBuffer;
}

void FreeFileBackedBuffer(IFileBackedBuffer* ptr)
{
    return delete ptr;
}
