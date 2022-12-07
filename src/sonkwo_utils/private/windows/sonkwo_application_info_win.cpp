#include "sonkwo_application_info.h"
#include "sonkwo_definition.h"
#include <string_convert.h>
#include <Windows.h>
#include <filesystem>
#include <memory>
const char regkey[] = "SOFTWARE\\sonkwo";
const char regkey32[] = "SOFTWARE\\WOW6432Node\\sonkwo";
const char COMMAND_PATH[] = "shell\\open\\command";

bool IsSonkwoAppStarted() {
    BOOL avaible = WaitNamedPipe(SONKWO_RUNTIME_PIPE, NMPWAIT_USE_DEFAULT_WAIT);
    return avaible > 0 ? 1 : 0;
}

bool UpdateInfoBySonkwoDir(const char* instdir)
{
	HKEY hKey;
	std::u16string u16str= sonkwo::U8ToU16(SONKWO_PRODUCT_NAME);
	if (RegCreateKeyExW(HKEY_CLASSES_ROOT, (LPCWSTR)u16str.c_str(), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == 0)
	{
		RegSetValueExW(hKey, NULL, NULL, REG_SZ, (BYTE*)L"URL:sonkwo", sizeof(L"URL:sonkwo")*2);
		RegSetValueExW(hKey, L"URL Protocol", NULL, REG_SZ, (BYTE*)L"", sizeof(L"") * 2);
		RegCloseKey(hKey);
	}
	else {
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError();

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
			(lstrlen((LPCTSTR)lpMsgBuf)  + 40) * sizeof(TCHAR));
		StringCchPrintf((LPTSTR)lpDisplayBuf,
			LocalSize(lpDisplayBuf),
			TEXT(" failed with error %d: %s"), dw, lpMsgBuf);
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
		return false;
	}

	std::filesystem::path keyypath(SONKWO_PRODUCT_NAME);
	keyypath /= COMMAND_PATH;
	if (RegCreateKeyExW(HKEY_CLASSES_ROOT, (LPCWSTR)keyypath.u16string().c_str(), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL ,&hKey,NULL ) == 0) {
		std::filesystem::path instpath((char8_t*)instdir);
		auto exepath = instpath / SONKWO_EXE_NAME;
		std::u16string u16command;
		u16command.append(u"\"");
		u16command.append(exepath.u16string());
		u16command.append(u"\"");
		u16command.append(u" -- \"%1\"");
		RegSetValueExW(hKey, NULL, NULL, REG_SZ, (BYTE*)u16command.c_str(), (u16command.size()+1)*2);
		RegCloseKey(hKey);
	}
	else {
		return false;
	}
	return true;
}

bool DeleteSonkwoInfo()
{
	HKEY hKey;
	std::u16string u16str = sonkwo::U8ToU16(SONKWO_PRODUCT_NAME);
	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, NULL, NULL, DELETE| KEY_ENUMERATE_SUB_KEYS| KEY_QUERY_VALUE, &hKey) == 0)
	{
		auto result=RegDeleteTreeW(hKey, (LPCWSTR)u16str.c_str());
		RegCloseKey(hKey);
		if (result != ERROR_SUCCESS) {
			LPVOID lpMsgBuf;
			LPVOID lpDisplayBuf;
			DWORD dw = result;

			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dw,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&lpMsgBuf,
				0, NULL);
			lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
				(lstrlen((LPCTSTR)lpMsgBuf) + 40) * sizeof(TCHAR));
			StringCchPrintf((LPTSTR)lpDisplayBuf,
				LocalSize(lpDisplayBuf),
				TEXT(" failed with error %d: %s"), dw, lpMsgBuf);
			MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

			return false;
		}
	}
	else {
		return false;
	}
	return true;
}

bool GetSonkwoDir(char *const* path,uint32_t* length) 
{
	if (!length) {
		return false;
	}

	HKEY hKey;
	std::filesystem::path keyypath(SONKWO_PRODUCT_NAME);
	keyypath /= COMMAND_PATH;
	DWORD dType = REG_SZ;
	LSTATUS result;
	uint32_t inbuflen = *length;
	*length = 0;
	uint32_t buflen=0;

	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, (LPCWSTR)keyypath.u16string().c_str(), NULL, KEY_READ, &hKey) == 0)
	{
		result=RegQueryValueExW(hKey, NULL, NULL, &dType, NULL, (LPDWORD)&buflen);
		if (result != ERROR_SUCCESS) {
			return false;
		}
		LPBYTE buf;
		buf =(LPBYTE) malloc(buflen);
		result = RegQueryValueExW(hKey, NULL, NULL, &dType, buf, (LPDWORD)&buflen);
		if (result != ERROR_SUCCESS) {
			return false;
		}
		std::u16string rawstr((char16_t*)buf);
		rawstr = rawstr.substr(rawstr.find(u"\"") + 1, rawstr.find(u"\"", rawstr.find(u"\"") + 1)-1);
		auto str = sonkwo::U16ToU8((const char16_t*)rawstr.c_str());
		free(buf);
		
		*length = str.size();
		if (str.size() > inbuflen) {
			return false;
		}
		memcpy(*path, str.c_str(), str.size());

		RegCloseKey(hKey);
	}
	else {
		return false;
	}
	return  true;
}

bool RunSonkwoClient()
{
	uint32_t buflength=0;
	GetSonkwoDir(nullptr, &buflength);
	if (buflength == 0) {
		return false;
	}
	char *const buf=(char*)malloc(sizeof(buflength));
	auto res = GetSonkwoDir(&buf, &buflength);
	std::filesystem::path sonkwoexepath((char8_t*)buf, (char8_t*)buf+ buflength);
	auto sonkwodir=sonkwoexepath.parent_path();
	//HINSTANCE sonkwohandle = ShellExecuteW(NULL, L"runas", path, strUnicode, NULL, SW_NORMAL);
	HINSTANCE sonkwohandle = ShellExecuteW(NULL, NULL, (LPCWSTR)sonkwoexepath.u16string().c_str(), L"", NULL, SW_NORMAL);

	if (!sonkwohandle)
	{
		DWORD err = GetLastError();
		return false;
	}
	return true;
}
