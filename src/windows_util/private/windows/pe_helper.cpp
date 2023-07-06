#include "pe_helper.h"
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <functional>
typedef struct {
    unsigned char Name[8];
    unsigned int VirtualSize;
    unsigned int VirtualAddress;
    unsigned int SizeOfRawData;
    unsigned int PointerToRawData;
    unsigned int PointerToRelocations;
    unsigned int PointerToLineNumbers;
    unsigned short NumberOfRelocations;
    unsigned short NumberOfLineNumbers;
    unsigned int Characteristics;
} sectionHeader;



int Rva2Offset(sectionHeader* sections,unsigned int NumberOfSections ,unsigned int rva) {
    int i = 0;

    for (i = 0; i < NumberOfSections; i++) {
        unsigned int x = sections[i].VirtualAddress + sections[i].SizeOfRawData;

        if (x >= rva) {
            return sections[i].PointerToRawData + (rva + sections[i].SizeOfRawData) - x;
        }
    }

    return -1;
}


PIMAGE_DOS_HEADER GetDosHeader(_In_ const char* pBase) {
	return PIMAGE_DOS_HEADER(pBase);
}

PIMAGE_NT_HEADERS GetNtHeader(_In_ const char* pBase) {
	return PIMAGE_NT_HEADERS(GetDosHeader(pBase)->e_lfanew + (SIZE_T)pBase);
}

PIMAGE_FILE_HEADER GetFileHeader(_In_ const char* pBase) {
	return &(GetNtHeader(pBase)->FileHeader);
}

PIMAGE_OPTIONAL_HEADER GetOptHeader(_In_ const char* pBase) {
	return &(GetNtHeader(pBase)->OptionalHeader);
}

PIMAGE_SECTION_HEADER GetLastSec(_In_ const char* pBase) {
	DWORD SecNum = GetFileHeader(pBase)->NumberOfSections;
	PIMAGE_SECTION_HEADER FirstSec = IMAGE_FIRST_SECTION(GetNtHeader(pBase));
	PIMAGE_SECTION_HEADER LastSec = FirstSec + SecNum - 1;
	return LastSec;
}

PIMAGE_SECTION_HEADER GetSecByName(_In_ const char* pBase, _In_ const char* name) {
	DWORD Secnum = GetFileHeader(pBase)->NumberOfSections;
	PIMAGE_SECTION_HEADER Section = IMAGE_FIRST_SECTION(GetNtHeader(pBase));
	char buf[10] = { 0 };
	for (DWORD i = 0; i < Secnum; i++) {
		memcpy_s(buf, 8, (char*)Section[i].Name, 8);
		if (!strcmp(buf, name)) {
			return Section + i;
		}
	}
	return nullptr;
}

PIMAGE_SECTION_HEADER FindSecByVirtualAddress(const char* pBase, DWORD address)
{
    DWORD Secnum = GetFileHeader(pBase)->NumberOfSections;
    PIMAGE_SECTION_HEADER Section = IMAGE_FIRST_SECTION(GetNtHeader(pBase));
    for (DWORD i = 0; i < Secnum; i++) {
        if (Section[i].VirtualAddress <= address && Section[i].VirtualAddress + Section[i].Misc.VirtualSize >= address) {
            return Section+i;
        }
    }
    return nullptr;
}

char* AddSec(_In_ char*& hpe, _In_ DWORD& filesize, _In_ const char* secname, _In_ const int secsize) {
	GetFileHeader(hpe)->NumberOfSections++;
	PIMAGE_SECTION_HEADER pesec = GetLastSec(hpe);
	//设置区段表属性
	memcpy(pesec->Name, secname, 8);
	pesec->Misc.VirtualSize = secsize;
	pesec->VirtualAddress = (pesec - 1)->VirtualAddress + AlignMent((pesec - 1)->SizeOfRawData, GetOptHeader(hpe)->SectionAlignment);
	pesec->SizeOfRawData = AlignMent(secsize, GetOptHeader(hpe)->FileAlignment);
	pesec->PointerToRawData = AlignMent(filesize, GetOptHeader(hpe)->FileAlignment);
	pesec->Characteristics = 0xE00000E0;
	//设置OPT头映像大小
	GetOptHeader(hpe)->SizeOfImage = pesec->VirtualAddress + pesec->SizeOfRawData;
	//扩充文件数据
	int newSize = pesec->PointerToRawData + pesec->SizeOfRawData;
	char* nhpe = new char [newSize] {0};
	//向新缓冲区录入数据
	memcpy(nhpe, hpe, filesize);
	//缓存区更替
	delete hpe;
	filesize = newSize;
	return nhpe;
}

void FixStub(char* targetDllbase, char* stubDllbase, DWORD targetNewScnRva, DWORD stubTextRva)
{
	//找到stub.dll的重定位表
	DWORD dwRelRva = GetOptHeader(stubDllbase)->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    DWORD toltalSize = GetOptHeader(stubDllbase)->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
	IMAGE_BASE_RELOCATION* pRel = (IMAGE_BASE_RELOCATION*)(dwRelRva + stubDllbase);

	//遍历重定位表
	while (toltalSize>0)
	{
		struct TypeOffset
		{
			WORD offset : 12;
			WORD type : 4;

		};
		TypeOffset* pTypeOffset = (TypeOffset*)(pRel + 1);
		DWORD dwCount = (pRel->SizeOfBlock - 8) / 2;	//需要重定位的数量
		for (int i = 0; i < dwCount; i++)
		{
            switch (pTypeOffset[i].type) {
            case IMAGE_REL_BASED_HIGHLOW: {
                //需要重定位的地址
                DWORD* pFixAddrdw = (DWORD*)(pRel->VirtualAddress + pTypeOffset[i].offset + stubDllbase);
                DWORD dwOld;
                char** pFixAddr =(char**) pFixAddrdw;
                //修改属性为可写
                VirtualProtect(pFixAddr, 4, PAGE_READWRITE, &dwOld);
                //去掉dll当前加载基址
                //换上目标文件的加载基址
                *pFixAddr += (targetDllbase - stubDllbase);
               
                //去掉默认的段首RVA
                *pFixAddr -= stubTextRva;

                //加上新区段的段首RVA
                *pFixAddr += targetNewScnRva;
                //把属性修改回去
                VirtualProtect(pFixAddr, 4, dwOld, &dwOld);
                break;
            }
            case IMAGE_REL_BASED_DIR64: {
                //需要重定位的地址
                ULONGLONG* pFixAddrdw = (ULONGLONG*)(pRel->VirtualAddress + pTypeOffset[i].offset + stubDllbase);
                DWORD dwOld;
                char** pFixAddr = (char**)pFixAddrdw;
                VirtualProtect(pFixAddr, 8, PAGE_READWRITE, &dwOld);
                *pFixAddr += (targetDllbase - stubDllbase);
                *pFixAddr -= stubTextRva;
                *pFixAddr += targetNewScnRva;
                VirtualProtect(pFixAddr, 8, dwOld, &dwOld);
                break;
            }
            default:
                break;
            }
		}
		//切换到下一个重定位块
        toltalSize -= pRel->SizeOfBlock;
		pRel = (IMAGE_BASE_RELOCATION*)((char*)pRel + pRel->SizeOfBlock);
        
	}

}

class ResourceDirectoryIterator {
    typedef std::function<void(PIMAGE_RESOURCE_DATA_ENTRY)> FnIterCallback;
public:
    
    void StartIter(PIMAGE_RESOURCE_DIRECTORY _root, FnIterCallback fn) {
        root = _root;
        callback = fn;
        IterResDir(root);
    }
private:
    void IterResDir( PIMAGE_RESOURCE_DIRECTORY resDir) {
        PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry = PIMAGE_RESOURCE_DIRECTORY_ENTRY(resDir + 1);
        int i = 0, j = 0;
        for (; i < resDir->NumberOfNamedEntries; i++) {
            ParseResDirEntry( resDirEntry + i);
        }
        for (; j < resDir->NumberOfIdEntries; j++) {
            ParseResDirEntry( resDirEntry + i + j);
        }
    };
    void ParseResDirEntry( PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry) {
        if (resDirEntry->DataIsDirectory) {
            PIMAGE_RESOURCE_DIRECTORY resDir = PIMAGE_RESOURCE_DIRECTORY((uintptr_t)root + resDirEntry->OffsetToDirectory);
            IterResDir(resDir);
        }
        else {
            PIMAGE_RESOURCE_DATA_ENTRY resDir = PIMAGE_RESOURCE_DATA_ENTRY((uintptr_t)root + resDirEntry->OffsetToData);
            callback(resDir);
        }
    }
    PIMAGE_RESOURCE_DIRECTORY root;
    FnIterCallback  callback;
};


void ModResourceDirectory(char* srcImage, IMAGE_SECTION_HEADER& desSection)
{
    DWORD srcResVirtualAddress = GetOptHeader(srcImage)->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
    DWORD srcResSize = GetOptHeader(srcImage)->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
    if (!srcResSize) {
        return;
    }
    PIMAGE_SECTION_HEADER srcSec=FindSecByVirtualAddress(srcImage,srcResVirtualAddress);
    if (!srcSec) {
        return;
    }

    auto SrcRvaToRaw = [&](DWORD rva)-> DWORD {
        PIMAGE_SECTION_HEADER sec = FindSecByVirtualAddress(srcImage, rva);
        if (sec != srcSec) {
            printf("file Resource Directory bad format");
            exit(1);
        }
        return  rva - sec->VirtualAddress + sec->PointerToRawData;
    };

    PIMAGE_RESOURCE_DIRECTORY resDir=PIMAGE_RESOURCE_DIRECTORY (srcImage+SrcRvaToRaw(srcResVirtualAddress));
    ResourceDirectoryIterator resourceDirectoryIterator;
    resourceDirectoryIterator.StartIter(resDir, [&](PIMAGE_RESOURCE_DATA_ENTRY entry) {
        entry->OffsetToData = entry->OffsetToData - srcSec->VirtualAddress + desSection.VirtualAddress;
        });
}


PPEB get_peb()
{
#if defined(_M_X64) // x64
    PTEB tebPtr = (PTEB)__readgsqword(offsetof(NT_TIB, Self));
#else // x86
    PTEB tebPtr = (PTEB)__readfsdword(offsetof(NT_TIB, Self));
#endif
    return tebPtr->ProcessEnvironmentBlock;
}




void EnumExportedFunctions(const char* szFilename, void (*callback)(const char*)) {
    FILE* hFile = fopen(szFilename, "rb");
    sectionHeader* sections;
    unsigned int NumberOfSections = 0;
    if (hFile != NULL) {
        if (fgetc(hFile) == 'M' && fgetc(hFile) == 'Z') {
            unsigned int e_lfanew = 0;
            unsigned int NumberOfRvaAndSizes = 0;
            unsigned int ExportVirtualAddress = 0;
            unsigned int ExportSize = 0;
            int i = 0;

            fseek(hFile, 0x3C, SEEK_SET);
            fread(&e_lfanew, 4, 1, hFile);
            fseek(hFile, e_lfanew + 6, SEEK_SET);
            fread(&NumberOfSections, 2, 1, hFile);
            fseek(hFile, 108, SEEK_CUR);
            fread(&NumberOfRvaAndSizes, 4, 1, hFile);

            if (NumberOfRvaAndSizes == 16) {
                fread(&ExportVirtualAddress, 4, 1, hFile);
                fread(&ExportSize, 4, 1, hFile);

                if (ExportVirtualAddress > 0 && ExportSize > 0) {
                    fseek(hFile, 120, SEEK_CUR);

                    if (NumberOfSections > 0) {
                        sections = (sectionHeader*)malloc(NumberOfSections * sizeof(sectionHeader));

                        for (i = 0; i < NumberOfSections; i++) {
                            fread(sections[i].Name, 8, 1, hFile);
                            fread(&sections[i].VirtualSize, 4, 1, hFile);
                            fread(&sections[i].VirtualAddress, 4, 1, hFile);
                            fread(&sections[i].SizeOfRawData, 4, 1, hFile);
                            fread(&sections[i].PointerToRawData, 4, 1, hFile);
                            fread(&sections[i].PointerToRelocations, 4, 1, hFile);
                            fread(&sections[i].PointerToLineNumbers, 4, 1, hFile);
                            fread(&sections[i].NumberOfRelocations, 2, 1, hFile);
                            fread(&sections[i].NumberOfLineNumbers, 2, 1, hFile);
                            fread(&sections[i].Characteristics, 4, 1, hFile);
                        }

                        unsigned int NumberOfNames = 0;
                        unsigned int AddressOfNames = 0;

                        int offset = Rva2Offset(sections, NumberOfSections,ExportVirtualAddress);
                        fseek(hFile, offset + 24, SEEK_SET);
                        fread(&NumberOfNames, 4, 1, hFile);

                        fseek(hFile, 4, SEEK_CUR);
                        fread(&AddressOfNames, 4, 1, hFile);

                        unsigned int namesOffset = Rva2Offset(sections, NumberOfSections, AddressOfNames), pos = 0;
                        fseek(hFile, namesOffset, SEEK_SET);

                        for (i = 0; i < NumberOfNames; i++) {
                            unsigned int y = 0;
                            fread(&y, 4, 1, hFile);
                            pos = ftell(hFile);
                            fseek(hFile, Rva2Offset(sections, NumberOfSections, y), SEEK_SET);

                            char c = fgetc(hFile);
                            int szNameLen = 0;

                            while (c != '\0') {
                                c = fgetc(hFile);
                                szNameLen++;
                            }

                            fseek(hFile, (-szNameLen) - 1, SEEK_CUR);
                            char* szName = (char*)calloc(szNameLen + 1, 1);
                            fread(szName, szNameLen, 1, hFile);

                            callback(szName);
                            free(szName);
                            fseek(hFile, pos, SEEK_SET);
                        }
                        free(sections);
                    }
                }
            }
        }
        
        fclose(hFile);
    }
}





void EnumExportedFunctionsHandle(HMODULE handle, EnumExportedCallback callback) {
    const char* content = (const char*)handle;
    const char* cur = content, pos = 0;
    unsigned int NumberOfSections = 0;
    if (content == NULL) {
        return;
    }
    if (memcmp("MZ", cur, strlen("MZ")) != 0) {
        return;
    }
    PIMAGE_EXPORT_DIRECTORY pexDirectory = (PIMAGE_EXPORT_DIRECTORY)(GetNtHeader(content)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + content);
    const uint32_t* funcNameOffsets = (const uint32_t*)(content + pexDirectory->AddressOfNames);
    const uint16_t* funcOrdinals = (const uint16_t*)(content + pexDirectory->AddressOfNameOrdinals);
    const uint32_t* funcOffsets = (const uint32_t*)(content + pexDirectory->AddressOfFunctions);
    for (int i = 0; i < pexDirectory->NumberOfNames; i++) {
       
        const char* funcname = content+ funcNameOffsets[i];
        uint16_t ordinal = funcOrdinals[i];
        if (!callback(funcname, (void*)(content + funcOffsets[ordinal]))) {
            break;
        }
    }

    //unsigned int e_lfanew = 0;
    //unsigned int NumberOfRvaAndSizes = 0;
    //unsigned int ExportVirtualAddress = 0;
    //unsigned int ExportSize = 0;
    //int i = 0;
    //cur = content + 0x3C;
    //memcpy(&e_lfanew, cur, 4);
    //cur = content + e_lfanew + 6;
    //memcpy(&NumberOfSections, cur, 2);
    //cur += 2;
    //cur += 108;
    //memcpy(&NumberOfRvaAndSizes, cur, 4);
    //cur += 4;
    //if (NumberOfRvaAndSizes == 16) {
    //    memcpy(&ExportVirtualAddress, cur, 4);
    //    cur += 4;
    //    memcpy(&ExportSize, cur, 4);
    //    cur += 4;

    //    if (ExportVirtualAddress > 0 && ExportSize > 0) {
    //        cur += 120;
    //        if (NumberOfSections > 0) {

    //            unsigned int NumberOfNames = 0;
    //            unsigned int AddressOfNames = 0;

    //            //int offset = Rva2Offset(ExportVirtualAddress);
    //            //cur = content + offset + 24;
    //            cur = content + ExportVirtualAddress + 24;
    //            memcpy(&NumberOfNames, cur, 4);
    //            cur += 4;
    //            cur += 4;
    //            memcpy(&AddressOfNames, cur, 4);

    //            //unsigned int namesOffset = Rva2Offset(AddressOfNames), pos = 0;
    //            //fseek(hFile, namesOffset, SEEK_SET);
    //            cur = content + AddressOfNames;

    //            const char* pos = cur;
    //            for (i = 0; i < NumberOfNames; i++) {
    //                unsigned int y = 0;
    //                memcpy(&y, cur, 4);
    //                cur += 4;
    //                pos = cur;
    //                cur = content + y;

    //                int szNameLen = 0;
    //                while (*cur++ != '\0') {
    //                    szNameLen++;
    //                }
    //                cur -= (szNameLen + 1);
    //                char* szName = (char*)calloc(szNameLen + 1, 1);
    //                memcpy(szName, cur, szNameLen);

    //                callback(szName);
    //                free(szName);
    //                cur = pos;
    //            }
    //        }
    //    }
    //}
}
