#include <stdint.h>
#define S_INFO_BUFFER_SIZE 32767

#define S_GENERIC_READ 0x00000001L
#define S_GENERIC_WRITE 0x00000002L
#define S_GENERIC_EXECUTE 0x00000004L
#define S_GENERIC_ALL  (S_GENERIC_READ|S_GENERIC_WRITE|S_GENERIC_EXECUTE)
#ifdef __cplusplus
extern "C" {
#endif
    enum EUserType { LocalUser, ProcessUser,Users};

    //ChangeAccessControl();

    bool GetFolderAccessControlForUser(const char* path, EUserType type,uint64_t* mask);
    bool GrantFolderAccessControlForUser(const char* path, EUserType type, uint64_t mask);
#ifdef __cplusplus
}
#endif