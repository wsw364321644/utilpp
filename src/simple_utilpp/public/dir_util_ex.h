#include <stdint.h>
#include <system_error>
#include "PathBuf.h"
#include "simple_export_ppdefs.h"
#define S_INFO_BUFFER_SIZE 32767

#define S_GENERIC_READ 0x00000001L
#define S_GENERIC_WRITE 0x00000002L
#define S_GENERIC_EXECUTE 0x00000004L
#define S_GENERIC_ALL  (S_GENERIC_READ|S_GENERIC_WRITE|S_GENERIC_EXECUTE)

enum EUserType { LocalUser, ProcessUser,Users};

//ChangeAccessControl();

SIMPLE_UTIL_EXPORT bool GetFolderAccessControlForUser(std::u8string_view  path, EUserType type, uint64_t* mask,std::error_code& ec);
SIMPLE_UTIL_EXPORT bool GetFolderAccessControlForUser(FPathBuf&  path, EUserType type, uint64_t* mask,std::error_code& ec);
SIMPLE_UTIL_EXPORT bool GrantFolderAccessControlForUser(std::u8string_view  path, EUserType type, uint64_t mask, std::error_code& ec);
SIMPLE_UTIL_EXPORT bool GrantFolderAccessControlForUser(FPathBuf& path, EUserType type, uint64_t mask, std::error_code& ec);


