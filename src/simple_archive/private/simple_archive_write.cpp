#include "simple_archive_write.h"
#include <simple_error.h>
#include <FunctionExitHelper.h>
#include <char_buffer_extension.h>
#include <dir_util.h>
#include <wildmatch.h>
#include <archive.h>
#include <archive_entry.h>
void FArchiveWriteHelper::ArchiveFolder(FPathBuf& dir, std::error_code& ec)
{
    std::filesystem::path outFile = dir.GetU8View();
    outFile += ".zip";
    ArchiveFolder(dir, outFile.u8string(), ec);
}

void FArchiveWriteHelper::ArchiveFolder(FPathBuf& dir, std::u8string_view archiveFilePath, std::error_code& ec)
{
    int ires;
    FCharBuffer& charBuf= *FCharBuffer::GetThreadSingleton();
    struct archive_entry* entry;
    FRawFile file;
    entry = archive_entry_new();
    if (!entry) {
        ec = std::make_error_code(std::errc::no_space_on_device);
        return;
    }
    FunctionExitHelper_t entryGuarder(
        [&]() {
            archive_entry_free(entry);
        }
    );
    struct archive* a = archive_write_new();
    if (!a) {
        ec = std::make_error_code(std::errc::no_space_on_device);
        return;
    }
    FunctionExitHelper_t archiverGuarder(
        [&]() {
            archive_write_free(a);
        }
    );
    ires = archive_write_set_format_zip(a);
    if (ires != ARCHIVE_OK) {
        ec = utilpp::make_common_used_error(utilpp::ECommonUsedError::CUE_UNKNOW);
        return;
    }

    auto strPtr = GetStringViewCStr(ConvertU8ViewToView(archiveFilePath), charBuf);
    ires = archive_write_open_filename(a, strPtr);
    if (ires != ARCHIVE_OK) {
        ec = utilpp::make_common_used_error(utilpp::ECommonUsedError::CUE_UNKNOW);
        return;
    }
    FunctionExitHelper_t GuardArchiverOpen(
        [&]() {
            archive_write_close(a);
        }
    );
    charBuf.Reverse(1 << 13);
    DirUtil::IterateDirRecursively(dir,
        [&](DirEntry_t& dirEntry,bool& bExit,bool& bEnter) {
            bool bExclude{ false };
            for (auto& ExcludeFile : ExcludeFiles) {
                std::string filePath;
                filePath = dirEntry.pPathBuf->GetView();
                std::replace(filePath.begin(), filePath.end(), '\\', '/');
                if (wildmatch(ExcludeFile.c_str(), filePath.c_str(), WM_WILDSTAR) == WM_MATCH) {
                    bEnter = false;
                    bExclude = true;
                    break;
                }
            }
            if (dirEntry.bDir) {
                return;
            }
            if (bExclude) {
                return;
            }

            ires = file.Open(*dirEntry.pPathBuf, UTIL_OPEN_EXISTING);
            if (ires != ERR_SUCCESS) {
                ec = std::make_error_code(std::errc::io_error);
                bExit = true;
                return;
            }
            auto relatePath = dirEntry.pPathBuf->PopPath(dirEntry.Depth + 1);
            FunctionExitHelper_t dirGuard(
                [&]() {
                    dirEntry.pPathBuf->AppendPath(relatePath);
                }
            );
            archive_entry_clear(entry);
            archive_entry_set_pathname(entry, (const char*)relatePath.data());
            archive_entry_set_size(entry, dirEntry.Size);
            archive_entry_set_filetype(entry, AE_IFREG);
            archive_entry_set_perm(entry, 0644);
            archive_write_header(a, entry);

            uint32_t readed{ std::numeric_limits<uint32_t>::max() };

            while (ires == ERR_SUCCESS && readed != 0) {
                ires = file.Read(charBuf.Data(), charBuf.Capacity(), readed);
                auto writed = archive_write_data(a, charBuf.Data(), readed);
                if (writed != readed) {
                    ec = std::make_error_code(std::errc::io_error);
                    bExit = true;
                    return;
                }
            }
            return;
        }
    );
}

void FArchiveWriteHelper::AddExcludeFile(std::string_view pathstr)
{
    ExcludeFiles.emplace_back(pathstr);
    auto& str=ExcludeFiles.back();
    if (str.find('/') == std::string::npos && str.find('\\') == std::string::npos) {
        str = "**/" + str;
    }
}

void FArchiveWriteHelper::ClearExcludeFiles(std::string_view pathstr)
{
    ExcludeFiles.clear();
}
