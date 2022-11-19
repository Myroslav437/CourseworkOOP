// Compile only for Windows-like systems:
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#include <string>

#include <Windows.h>
#include <wingdi.h>

#include "../screencap.hpp"

namespace scap
{

Screencap::pixelmap_t 
Screencap::__get_display_dump__(std::size_t* w, std::size_t* h, const std::string& dname) 
{
    HDC hScreen;

    if(dname.empty()) {
        hScreen = GetDC(NULL);
    }
    else {
        hScreen = CreateDCA(dname.c_str(), NULL, NULL, NULL);
    }

    if(hScreen == NULL) {
        throw std::runtime_error("__get_display_dump__(std::size_t* w, std::size_t* h) : The failed to get the device context:"); 
    }

    const auto ScreenX = GetDeviceCaps(hScreen, HORZRES);
    const auto ScreenY = GetDeviceCaps(hScreen, VERTRES);

    // Set resulting height and weight:
    *w = ScreenX;
    *h = ScreenY;

    HDC hdcMem = CreateCompatibleDC(hScreen);
    if(hdcMem == NULL) {
        throw std::runtime_error("__get_display_dump__(std::size_t* w, std::size_t* h) : CreateCompatibleDC() failed:");
    }

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, ScreenX, ScreenY);
    if(hBitmap == NULL) {
        throw std::runtime_error("__get_display_dump__(std::size_t* w, std::size_t* h) : CreateCompatibleBitmap() failed:");
    }

    HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);
    BitBlt(hdcMem, 0, 0, ScreenX, ScreenY, hScreen, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld);

    BITMAPINFOHEADER bmi = {0};
    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biPlanes = 1;
    bmi.biBitCount = 32;
    bmi.biWidth = ScreenX;
    bmi.biHeight = -ScreenY;
    bmi.biCompression = BI_RGB;
    bmi.biSizeImage = 0;// 3 * ScreenX * ScreenY;

    
    pixelmap_t ScreenData (ScreenX * ScreenY * 4); // 32bits deep not 24

    if(GetDIBits(hdcMem, hBitmap, 0, ScreenY, ScreenData.data(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS)
        == 0)
    {
        throw std::runtime_error("__get_display_dump__(std::size_t* w, std::size_t* h) : GetDIBits() failed:");
    }

    // Convert to 24bits depth:
    pixelmap_t pmap(ScreenX * ScreenY * 3);

    for (unsigned int y = 0; y < ScreenY ; ++y) {
        for (unsigned int x = 0; x < ScreenX; ++x) {
            const auto blue =  static_cast<unsigned char>(ScreenData[4*((y*ScreenX)+x)]);
            const auto green = static_cast<unsigned char>(ScreenData[4*((y*ScreenX)+x)+1]);
            const auto red =   static_cast<unsigned char>(ScreenData[4*((y*ScreenX)+x)+2]);

            pmap[(x + y * ScreenX) * 3] = red;
            pmap[(x + y * ScreenX) * 3 + 1] = green;
            pmap[(x + y * ScreenX) * 3 + 2] = blue;
        }
    }

    ReleaseDC(GetDesktopWindow(),hScreen);
    DeleteDC(hdcMem);
    DeleteObject(hBitmap);


    return pmap;
}

} // namespace scap

#endif