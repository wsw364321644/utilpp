#pragma once
#include "simple_archive_export_defs.h"
#include <PathBuf.h>
#include <system_error>
class SIMPLE_ARCHIVE_EXPORT FArchiveWriteHelper {
public:
    void ArchiveFolder(FPathBuf& dir, std::error_code& ec);
    void ArchiveFolder(FPathBuf& dir,std::u8string_view archiveFilePath, std::error_code& ec);
    void AddExcludeFile(std::string_view pathstr);
    void ClearExcludeFiles(std::string_view pathstr);
private:
    std::vector<std::string> ExcludeFiles;
};
