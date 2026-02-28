#include "dir_util_ex.h"
#include "dir_util_internal.h"
#include <FunctionExitHelper.h>
#include <string_convert.h>
#include <simple_os_defs.h>
#include <stdio.h>
#include <aclapi.h>
#include <tchar.h>
#include <strsafe.h>
#include <authz.h>
#include <Sddl.h> 
#include <stdlib.h>
#define SECURITY_WIN32 
#include <security.h> 
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "authz.lib")

LPTSTR lpServerName = NULL;

bool GetLocalUserName(char* const buff,uint64_t * len)
{
    DWORD  bufCharCount = S_INFO_BUFFER_SIZE;
    auto res= GetUserNameExA(NameUnknown,buff, &bufCharCount);
    *len = bufCharCount;
    return res;
}



PSID ConvertNameToBinarySid(LPCSTR pAccountName)
{
    LPTSTR pDomainName = NULL;
    DWORD dwDomainNameSize = 0;
    PSID pSid = NULL;
    DWORD dwSidSize = 0;
    SID_NAME_USE sidType;
    BOOL fSuccess = FALSE;
    HRESULT hr = S_OK;

    __try
    {
        LookupAccountNameA(
            lpServerName,      // look up on local system
            pAccountName,
            pSid,              // buffer to receive name
            &dwSidSize,
            pDomainName,
            &dwDomainNameSize,
            &sidType);

        //  If the Name cannot be resolved, LookupAccountName will fail with
        //  ERROR_NONE_MAPPED.
        if (GetLastError() == ERROR_NONE_MAPPED)
        {
            //wprintf_s(_T("LookupAccountName failed with %d\n"), GetLastError());
            __leave;
        }
        else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pSid = (LPTSTR)LocalAlloc(LPTR, dwSidSize * sizeof(TCHAR));
            if (pSid == NULL)
            {
                //wprintf_s(_T("LocalAlloc failed with %d\n"), GetLastError());
                __leave;
            }

            pDomainName = (LPTSTR)LocalAlloc(LPTR, dwDomainNameSize * sizeof(TCHAR));
            if (pDomainName == NULL)
            {
                //wprintf_s(_T("LocalAlloc failed with %d\n"), GetLastError());
                __leave;
            }

            if (!LookupAccountNameA(
                lpServerName,      // look up on local system
                pAccountName,
                pSid,              // buffer to receive name
                &dwSidSize,
                pDomainName,
                &dwDomainNameSize,
                &sidType))
            {
                //wprintf_s(_T("LookupAccountName failed with %d\n"), GetLastError());
                __leave;
            }
        }

        //  Any other error code
        else
        {
            //wprintf_s(_T("LookupAccountName failed with %d\n"), GetLastError());
            __leave;
        }

        fSuccess = TRUE;
    }
    __finally
    {
        if (pDomainName != NULL)
        {
            LocalFree(pDomainName);
            pDomainName = NULL;
        }

        //  Free pSid only if failed;
        //  otherwise, the caller has to free it after use.
        if (fSuccess == FALSE)
        {
            if (pSid != NULL)
            {
                LocalFree(pSid);
                pSid = NULL;
            }
        }
    }

    return pSid;
}


LPSTR GetUserSIDStr(EUserType userType) {
    LPSTR str{NULL};
    switch (userType) {
    case EUserType::LocalUser: {
        char* UserName = (char*)malloc(S_INFO_BUFFER_SIZE);
        uint64_t count;
        if (GetLocalUserName(UserName, &count)) {
            auto psid = ConvertNameToBinarySid(UserName);
            if (psid) {
                ConvertSidToStringSidA(psid, &str);
                LocalFree(psid);
            }
        }
        free(UserName);
        break;
    }
    case EUserType::ProcessUser: {

        HANDLE hToken{ NULL };
        DWORD dwLength;
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
        PTOKEN_USER ptUser{};
        if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLength)) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                break;
            }
            ptUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, dwLength);
        }
        else {
            break;
        }
        if (GetTokenInformation(hToken, TokenUser, (LPVOID)ptUser, dwLength, &dwLength))
        {
            ConvertSidToStringSidA(ptUser->User.Sid, &str);
        }
        HeapFree(GetProcessHeap(), 0, ptUser);
        break;
    }
    case EUserType::Users: {
        PSID pSid{ 0 };
        LPSTR ppsidstr = GetUserSIDStr(EUserType::ProcessUser);
        PSID domainSid{ 0 };
        DWORD sidSize;
        PSID usersSid{0};
        auto clean = [&]() {
            if (pSid) {
                LocalFree(pSid);
            }
            if (ppsidstr) {
                LocalFree(ppsidstr);
            }
            if (domainSid) {
                free(domainSid);
            }
            if (usersSid) {
                free(usersSid);
            }
        };
        if (!ppsidstr) {
            clean();
            break;
        }
        ConvertStringSidToSidA(ppsidstr, &pSid);
        if (!pSid) {
            clean();
            break;
        }

        GetWindowsAccountDomainSid(pSid, NULL, &sidSize);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER && GetLastError() != ERROR_INVALID_PARAMETER) {
            clean();
            break;
        }
        domainSid = malloc(sidSize);
        if (!GetWindowsAccountDomainSid(pSid, domainSid, &sidSize)) {
            clean();
            break;
        }

        CreateWellKnownSid(WELL_KNOWN_SID_TYPE::WinBuiltinUsersSid, domainSid, NULL, &sidSize);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER&& GetLastError() != ERROR_INVALID_PARAMETER) {
            auto err=GetLastError();
            clean();
            break;
        }
        usersSid = malloc(sidSize);
        if (!CreateWellKnownSid(WELL_KNOWN_SID_TYPE::WinBuiltinUsersSid, domainSid, usersSid, &sidSize)) {
            clean();
            break;
        }
        ConvertSidToStringSidA(usersSid, &str);
        clean();
    }

    }
    return str;
}

bool GetUserNameByType(char* const name, uint64_t* size,EUserType userType) {
    switch (userType) {
    case EUserType::LocalUser: {
        return GetLocalUserName(name, size);
        break;
    }
    case EUserType::ProcessUser: {

        HANDLE hToken{ NULL };
        DWORD dwLength;
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
        PTOKEN_USER ptUser{};
        bool res{false};
        SID_NAME_USE SidType;
        if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLength)) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                break;
            }
            ptUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, dwLength);
        }
        else {
            return res;
        }
        if (GetTokenInformation(hToken, TokenUser, (LPVOID)ptUser, dwLength, &dwLength))
        {
            DWORD count = *size;
            DWORD domainnamelen{ 0 };
            char* domainname;
            if (!LookupAccountSidA(NULL, ptUser->User.Sid, name, &count, NULL, &domainnamelen, &SidType)) {
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {
                    domainname = (char*)malloc(domainnamelen);
                    res=LookupAccountSidA(NULL, ptUser->User.Sid, name, &count, domainname, &domainnamelen, &SidType);
                    *size = count;
                }
            }
        }

        HeapFree(GetProcessHeap(), 0, ptUser);
        return res;
        break;
    }
    case EUserType::Users: {
        PSID pSid;
        auto pstr = GetUserSIDStr(EUserType::Users);
        auto res = ConvertStringSidToSidA(pstr, &pSid);
        LocalFree(pstr);
        if (!res) {
            return res;
        }
        SID_NAME_USE SidType;
        DWORD count = *size;
        DWORD domainnamelen{ 0 };
        char* domainname;
        if (!LookupAccountSidA(NULL, pSid, name, &count, NULL, &domainnamelen, &SidType)) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                domainname = (char*)malloc(domainnamelen);
                res = LookupAccountSidA(NULL, pSid, name, &count, domainname, &domainnamelen, &SidType);
                *size = count;
            }
        }
        return res;
        break;
    }
    }
    return NULL;
}



uint64_t TransAccessMask(ACCESS_MASK Mask)
{
    // This evaluation of the ACCESS_MASK is an example. 
    // Applications should evaluate the ACCESS_MASK as necessary.
    uint64_t mask{0};
    //wprintf_s(L"Effective Allowed Access Mask : %8X\n", Mask);
    if (((Mask & GENERIC_ALL) == GENERIC_ALL)
        || ((Mask & FILE_ALL_ACCESS) == FILE_ALL_ACCESS)){
        //wprintf_s(L"Full Control\n");
        return S_GENERIC_ALL;
    }
    if (((Mask & GENERIC_READ) == GENERIC_READ)
        || ((Mask & FILE_GENERIC_READ) == FILE_GENERIC_READ)){
        //wprintf_s(L"Read\n");
        mask|= S_GENERIC_READ;
    }
    if (((Mask & GENERIC_WRITE) == GENERIC_WRITE)
        || ((Mask & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE)) {
        mask|= S_GENERIC_WRITE;
        //wprintf_s(L"Write\n");
    }
    if (((Mask & GENERIC_EXECUTE) == GENERIC_EXECUTE)
        || ((Mask & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE)){
        mask |= S_GENERIC_EXECUTE;
        //wprintf_s(L"Execute\n");
    }
    return mask;
}

uint64_t GetAccess(AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClient, PSECURITY_DESCRIPTOR psd)
{
    AUTHZ_ACCESS_REQUEST AccessRequest = { 0 };
    AUTHZ_ACCESS_REPLY AccessReply = { 0 };
    BYTE     Buffer[1024];
    BOOL bRes = FALSE;  // assume error

    //  Do AccessCheck.
    AccessRequest.DesiredAccess = MAXIMUM_ALLOWED;
    AccessRequest.PrincipalSelfSid = NULL;
    AccessRequest.ObjectTypeList = NULL;
    AccessRequest.ObjectTypeListLength = 0;
    AccessRequest.OptionalArguments = NULL;

    RtlZeroMemory(Buffer, sizeof(Buffer));
    AccessReply.ResultListLength = 1;
    AccessReply.GrantedAccessMask = (PACCESS_MASK)(Buffer);
    AccessReply.Error = (PDWORD)(Buffer + sizeof(ACCESS_MASK));


    if (!AuthzAccessCheck(0,
        hAuthzClient,
        &AccessRequest,
        NULL,
        psd,
        NULL,
        0,
        &AccessReply,
        NULL)) {
        //wprintf_s(_T("AuthzAccessCheck failed with %d\n"), GetLastError());
        return 0;
    }

    return TransAccessMask(*(PACCESS_MASK)(AccessReply.GrantedAccessMask));
}

//BOOL GetEffectiveRightsForUser(AUTHZ_RESOURCE_MANAGER_HANDLE hManager,
//    PSECURITY_DESCRIPTOR psd,
//    LPCSTR lpszUserName,
//    uint64_t* mask)
//{
//    PSID pSid = NULL;
//    BOOL bResult = FALSE;
//    LUID unusedId = { 0 };
//    AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext = NULL;
//
//    pSid = ConvertNameToBinarySid(lpszUserName);
//    if (pSid != NULL)
//    {
//        bResult = AuthzInitializeContextFromSid(0,
//            pSid,
//            hManager,
//            NULL,
//            unusedId,
//            NULL,
//            &hAuthzClientContext);
//        if (bResult)
//        {
//            *mask=GetAccess(hAuthzClientContext, psd);
//            AuthzFreeContext(hAuthzClientContext);
//        }
//        //else
//            //wprintf_s(_T("AuthzInitializeContextFromSid failed with %d\n"), GetLastError());
//    }
//    if (pSid != NULL)
//    {
//        LocalFree(pSid);
//        pSid = NULL;
//    }
//
//    return bResult;
//}



bool GetFolderAccessControlForUserInternal(PSECURITY_DESCRIPTOR psd,EUserType userType, uint64_t* mask) {
    AUTHZ_RESOURCE_MANAGER_HANDLE hManager;
    BOOL bResult = FALSE;
    PSID pSid = NULL;
    LUID unusedId = { 0 };
    AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext = NULL;
    LPSTR str;
    HANDLE hToken{ NULL };
    DWORD dwLength;
    PTOKEN_USER ptUser{};


    bResult = AuthzInitializeResourceManager(AUTHZ_RM_FLAG_NO_AUDIT,
        NULL, NULL, NULL, NULL, &hManager);
    if (!bResult)
    {
        return false;
    }

    switch (userType) {
    case EUserType::ProcessUser: {
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
        if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLength)) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                goto rmend;
            }
            ptUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, dwLength);
        }
        else {
            goto rmend;
        }
        bResult = GetTokenInformation(hToken, TokenUser, (LPVOID)ptUser, dwLength, &dwLength);
        HeapFree(GetProcessHeap(), 0, ptUser);
        if (bResult) {
            bResult = AuthzInitializeContextFromToken(0,
                hToken,
                hManager,
                NULL,
                unusedId,
                NULL,
                &hAuthzClientContext);
        }

        if (!bResult)
        {
            goto psidend;
        }
        break;
    }
    default: {
        str = GetUserSIDStr(userType);
        if (!str) {
            goto rmend;
        }
        bResult = ConvertStringSidToSidA(str, &pSid);
        LocalFree(str);

        if (!bResult || pSid == NULL)
        {
            bResult = false;
            goto rmend;
        }
        bResult = AuthzInitializeContextFromSid(0,
            pSid,
            hManager,
            NULL,
            unusedId,
            NULL,
            &hAuthzClientContext);
        if (!bResult)
        {
            goto psidend;
        }
        break;

    }
    }

    *mask = GetAccess(hAuthzClientContext, psd);
    AuthzFreeContext(hAuthzClientContext);


psidend:
    pSid = NULL;
    LocalFree(pSid);
rmend:
    AuthzFreeResourceManager(hManager);
    return bResult;
}

bool GetFolderAccessControlForUser(std::u8string_view  path, EUserType userType, uint64_t* mask, std::error_code& ec) {
    PathBuf.SetPath((char*)path.data(), path.size());
    return GetFolderAccessControlForUser(PathBuf, userType, mask,ec);
}
bool GetFolderAccessControlForUser(FPathBuf& pathBuf, EUserType userType, uint64_t* mask, std::error_code& ec)
{
    DWORD                dw;
    PACL                 pacl{ 0 };
    PSECURITY_DESCRIPTOR psd;
    ec.clear();
    pathBuf.ToPathW();
    auto pathw = pathBuf.GetPrependFileNamespacesW();
    dw = GetNamedSecurityInfoW(pathw, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION |
        OWNER_SECURITY_INFORMATION |
        GROUP_SECURITY_INFORMATION, NULL, NULL, &pacl, NULL, &psd);
    if (dw != ERROR_SUCCESS)
    {
        ec = std::error_code(dw, std::system_category());
        printf("GetNamedSecurityInfoW failed with %d\n", dw);
        return false;
    }
    //print ace in acl
//for (int i = 0;; i++) {
//    ACE_HEADER* ace;
//    ACCESS_MASK _mask;
//    if (!GetAce(pacl, i, (void**)&ace)) {
//        auto err = GetLastError();
//        break;
//    }
//    switch (ace->AceType) {
//    case ACCESS_ALLOWED_ACE_TYPE: {
//        pSid = &((ACCESS_ALLOWED_ACE*)ace)->SidStart;
//        _mask = ((ACCESS_ALLOWED_ACE*)ace)->Mask;
//        break;
//    }
//    case ACCESS_DENIED_ACE_TYPE: {
//        pSid = &((ACCESS_ALLOWED_ACE*)ace)->SidStart;
//        _mask = ((ACCESS_DENIED_ACE*)ace)->Mask;
//        break;
//    }
//    default:
//        break;
//    }
//    auto tmask = TransAccessMask(_mask);
//    SID_NAME_USE SidType;
//    char name[S_INFO_BUFFER_SIZE];
//    DWORD count = S_INFO_BUFFER_SIZE;
//    DWORD domainnamelen{ 0 };
//    char* domainname;
//    if (!LookupAccountSidA(NULL, pSid, name, &count, NULL, &domainnamelen, &SidType)) {
//        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
//        {
//            domainname = (char*)malloc(domainnamelen);
//            auto res = LookupAccountSidA(NULL, pSid, name, &count, domainname, &domainnamelen, &SidType);
//            printf("%s", name);
//            free(domainname);
//        }
//    }
//}
    bool bres = GetFolderAccessControlForUserInternal(psd, userType, mask);
    LocalFree(psd);
    return bres;
}



bool GrantFolderAccessControlForUser(std::u8string_view  path, EUserType userType, uint64_t mask, std::error_code& ec) {
    PathBuf.SetPath((char*)path.data(), path.size());
    return GrantFolderAccessControlForUser(PathBuf, userType, mask, ec);
}

bool GrantFolderAccessControlForUser(FPathBuf& pathBuf, EUserType type, uint64_t mask, std::error_code& ec)
{
    ec.clear();
    pathBuf.ToPathW();
    auto pathw = pathBuf.GetPrependFileNamespacesW();
    // Type of object, file or directory.  Here we test on directory
    SE_OBJECT_TYPE ObjectType = SE_FILE_OBJECT;
    // Access mask for new ACE equal to 0x001F0000 flags (bit 0 till 15)
    DWORD dwAccessRights = STANDARD_RIGHTS_ALL;
    // Type of ACE, Access denied ACE
    ACCESS_MODE AccessMode = GRANT_ACCESS;
    // Inheritance flags for new the ACE. The OBJECT_INHERIT_ACE and
    // CONTAINER_INHERIT_ACE flags are
    // not propagated to an inherited ACE.
    DWORD dwInheritance = NO_PROPAGATE_INHERIT_ACE;
    // format of trustee structure, the trustee is name
    TRUSTEE_FORM TrusteeForm = TRUSTEE_IS_NAME;
    // Trustee for new ACE.  This just for fun...When you run once, only one
    // element will take effect.  By changing the first array element we
    // can change to other trustee and re run the program....
    // Other than Mike spoon, they are all well known trustees
    // Take note the localization issues
    //WCHAR pszTrustee[4][15] = { L"Administrators", L"System", L"Users", L"Mike spoon"};

    // Result
    DWORD dwRes = 0;
    // Existing and new DACL pointers...
    PACL pOldDACL = NULL, pNewDACL = NULL;
    // Security descriptor
    PSECURITY_DESCRIPTOR pSD = NULL;
    SecureZeroMemory(&pSD, sizeof(PSECURITY_DESCRIPTOR));
    // EXPLICIT_ACCESS structure.  For more than one entries,
    // declare an array of the EXPLICIT_ACCESS structure
    EXPLICIT_ACCESS_A ea;
    // Verify the object name validity
    char* username = NULL;
    username = (char*)malloc(S_INFO_BUFFER_SIZE);
    uint64_t count = S_INFO_BUFFER_SIZE;
    wchar_t* interpath{NULL};
    if (pathBuf.PathLenW == 0)
    {
        ec = std::make_error_code(std::errc::invalid_argument);
        return false;
    }
    else {
        interpath=(wchar_t*)malloc((pathBuf.PathLenW +1)*sizeof(wchar_t));
        if (!interpath) {
            return false;
        }
        memcpy(interpath, pathw, pathBuf.PathLenW * sizeof(wchar_t));
        interpath[pathBuf.PathLenW] = L'\0';
    }
    FunctionExitHelper_t exithelper(
        [&]() {
            if (pSD != NULL)
                LocalFree((HLOCAL)pSD);
            if (pNewDACL != NULL)
                LocalFree((HLOCAL)pNewDACL);
            if (interpath) {
                free(interpath);
            }
            if (username) {
                free(username);
            }
        }
    );

    if (!GetUserNameByType(username, &count, LocalUser)) {
        return false;
    }

    // Get a pointer to the existing DACL.
    dwRes = GetNamedSecurityInfoW(interpath, ObjectType,
        DACL_SECURITY_INFORMATION,
        NULL,
        NULL,
        &pOldDACL,
        NULL,
        &pSD);


    if (dwRes != ERROR_SUCCESS)
    {
        ec = std::error_code(dwRes, std::system_category());
        return false;
    }

    // Initialize an EXPLICIT_ACCESS structure for the new ACE.
    // For more entries, declare an array of the EXPLICIT_ACCESS structure
    SecureZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = dwAccessRights;
    ea.grfAccessMode = AccessMode;
    ea.grfInheritance = dwInheritance;
    ea.Trustee.TrusteeForm = TrusteeForm;

    // Test for Administrators group, a new trustee for the ACE
    // For other trustees, you can try changing
    // the array index to 1, 2 and 3 and rerun, see the effect
    ea.Trustee.ptstrName = username;

    // Create a new ACL that merges the new ACE into the existing DACL.

    dwRes = SetEntriesInAclA(1, &ea, pOldDACL, &pNewDACL);
    if (dwRes != ERROR_SUCCESS)
    {
        ec = std::error_code(dwRes, std::system_category());
        return false;
    }

    // Attach the new ACL as the object's DACL.
    dwRes = SetNamedSecurityInfoW(interpath, ObjectType,
        DACL_SECURITY_INFORMATION,
        NULL,
        NULL,
        pNewDACL,
        NULL);
    if (dwRes != ERROR_SUCCESS)
    {
        ec = std::error_code(dwRes, std::system_category());
        return false;
    }
    return true;
}
