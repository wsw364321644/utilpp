#include "dir_util_ex.h"
#include "simple_error.h"
#include "dir_util_internal.h"

bool GetFolderAccessControlForUser(std::u8string_view path, EUserType type, uint64_t *mask, std::error_code &ec)
{
    ec = make_common_used_error(ECommonUsedError::CUE_NOT_SUPPORT);
    return false;
}
bool GetFolderAccessControlForUser(FPathBuf &path, EUserType type, uint64_t *mask, std::error_code &ec)
{
    return GetFolderAccessControlForUser(path.GetBuf(), type, mask, ec);
}
bool GrantFolderAccessControlForUser(std::u8string_view path, EUserType type, uint64_t mask, std::error_code &ec)
{
    ec = make_common_used_error(ECommonUsedError::CUE_NOT_SUPPORT);
    return false;
}
bool GrantFolderAccessControlForUser(FPathBuf &path, EUserType type, uint64_t mask, std::error_code &ec)
{
    return GrantFolderAccessControlForUser(path.GetBuf(), type, mask, ec);
}