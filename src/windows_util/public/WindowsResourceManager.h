#pragma once
#include <stdint.h>
#include <memory>
#include <simple_os_defs.h>
#include <gl/GL.h>
#include "windows_util_export_defs.h"
class WINDOWS_UTIL_EXPORT FWindowsResourceBitmap
{
public:

    ~FWindowsResourceBitmap();
    bool LoadResourceA(HINSTANCE hInstance, LPCSTR resourceName);
    void ReleaseResource();
    bool GenerateGLTexture();
    void RleaseGLTexture();
    int32_t Width;
    int32_t Height;
    uint8_t BitsPixel;
    int32_t WidthBytes;
    HANDLE hBM{ NULL };
    unsigned char* BMBits{ nullptr };
    GLuint GLTexture{ 0 };
};

class WINDOWS_UTIL_EXPORT FWindowsResourceManager
{
public:
    static std::shared_ptr<FWindowsResourceBitmap> LoadBitmapFromResourceA(HINSTANCE hInstance, LPCSTR resourceName);

};