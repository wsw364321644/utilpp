#include "dir_util_ex.h"
#include "simple_error.h"
#include "dir_util_internal.h"
namespace utilpp {
    namespace DirUtilEx {
        bool GetAccessControlForUser(std::u8string_view path, EUserType type, uint64_t *mask, std::error_code &ec)
        {
            ec = make_common_used_error(ECommonUsedError::CUE_NOT_SUPPORT);
            return false;
        }
        bool GetAccessControlForUser(FPathBuf &path, EUserType type, uint64_t *mask, std::error_code &ec)
        {
            return GetAccessControlForUser(path.GetBuf(), type, mask, ec);
        }
        bool GrantAccessControlForUser(std::u8string_view path, EUserType type, uint64_t mask, std::error_code &ec)
        {
            ec = make_common_used_error(ECommonUsedError::CUE_NOT_SUPPORT);
            return false;
        }
        bool GrantAccessControlForUser(FPathBuf &path, EUserType type, uint64_t mask, std::error_code &ec)
        {
            return GrantAccessControlForUser(path.GetBuf(), type, mask, ec);
        }

        bool GetVersionInfo(std::u8string_view path, FileVersionInfo_t& info, std::error_code& ec)
        {
            ec = make_common_used_error(ECommonUsedError::CUE_NOT_SUPPORT);
            return false;
        }
        bool GetVersionInfo(FPathBuf& path, FileVersionInfo_t& info, std::error_code& ec)
        {
            return GetVersionInfo(path.GetBuf(), info, ec);
        }

    }
}