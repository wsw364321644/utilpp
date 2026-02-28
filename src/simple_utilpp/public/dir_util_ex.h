#include <stdint.h>
#include <system_error>
#include "PathBuf.h"
#include "simple_export_ppdefs.h"

namespace utilpp {
    namespace DirUtilEx {
        enum EUserType { LocalUser, ProcessUser, Users };
        enum EFolderAccessMask :uint8_t
        {
            FAM_GENERIC_READ = 1,
            FAM_GENERIC_WRITE = 1 << 1,
            FAM_GENERIC_EXECUTE = 1 << 2,
            FAM_GENERIC_ALL = FAM_GENERIC_READ | FAM_GENERIC_WRITE | FAM_GENERIC_EXECUTE
        };
        SIMPLE_UTIL_EXPORT bool GetAccessControlForUser(std::u8string_view  path, EUserType type, uint64_t* mask, std::error_code& ec);
        SIMPLE_UTIL_EXPORT bool GetAccessControlForUser(FPathBuf& path, EUserType type, uint64_t* mask, std::error_code& ec);
        SIMPLE_UTIL_EXPORT bool GrantAccessControlForUser(std::u8string_view  path, EUserType type, uint64_t mask, std::error_code& ec);
        SIMPLE_UTIL_EXPORT bool GrantAccessControlForUser(FPathBuf& path, EUserType type, uint64_t mask, std::error_code& ec);


        typedef struct FileVersionInfo_t{
            uint64_t FileVersion{ 0 };
            uint64_t ProductVersion{ 0 };
            uint64_t FileDate{ 0 };
            bool bDebug : 1{false};
            bool bPreRelease : 1{false};
        }FileVersionInfo_t;
        SIMPLE_UTIL_EXPORT bool GetVersionInfo(std::u8string_view  path, FileVersionInfo_t& info, std::error_code& ec);
        SIMPLE_UTIL_EXPORT bool GetVersionInfo(FPathBuf& path, FileVersionInfo_t& info, std::error_code& ec);
    }
}
namespace DirUtilEx = utilpp::DirUtilEx;