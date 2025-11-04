#include "WindowsResourceManager.h"


FWindowsResourceBitmap::~FWindowsResourceBitmap()
{
    ReleaseResource();
}

bool FWindowsResourceBitmap::LoadResourceA(HINSTANCE hInstance, LPCSTR resourceName)
{
    hBM = ::LoadImageA(hInstance, resourceName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    BITMAP bmp;
    ::GetObjectA(hBM, sizeof(bmp), &bmp);
    Width = bmp.bmWidth;
    Height = bmp.bmHeight;
    BitsPixel = bmp.bmBitsPixel;
    WidthBytes = bmp.bmWidthBytes;
    BMBits = new unsigned char[WidthBytes * Height];
    for (int i = 0; i < Height; i++) {
        memcpy(BMBits + i * WidthBytes, (char*)bmp.bmBits + (Height - 1 - i) * WidthBytes, WidthBytes);
    }

    return true;
}

void FWindowsResourceBitmap::ReleaseResource()
{
    if (hBM) {
        ::DeleteObject(hBM);
    }
    if (BMBits) {
        delete[] BMBits;
    }
}

bool FWindowsResourceBitmap::GenerateGLTexture()
{

    glGenTextures(1, &GLTexture);
    glBindTexture(GL_TEXTURE_2D, GLTexture);
    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, BMBits);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, BMBits);
    return true;
}

void FWindowsResourceBitmap::RleaseGLTexture()
{
    glDeleteTextures(1, &GLTexture);
}

std::shared_ptr<FWindowsResourceBitmap> FWindowsResourceManager::LoadBitmapFromResourceA(HINSTANCE hInstance, LPCSTR resourceName)
{
    auto ptr = std::make_shared<FWindowsResourceBitmap>();

    if (ptr->LoadResourceA(hInstance, resourceName)) {
        return ptr;
    }
    return nullptr;
}

