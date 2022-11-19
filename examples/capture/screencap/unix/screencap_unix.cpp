// Compile only for UNIX-like systems:
#if defined(unix) || defined(__unix__) || defined(__unix)

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>

#include <stdexcept>

#include "../screencap.hpp"

namespace scap
{

Screencap::pixelmap_t 
Screencap::__get_display_dump__(std::size_t* w, std::size_t* h, const std::string& dname) {

    // connect to the X server
    // Set Null for build
    Display* display;
    if(dname.empty()) {
        display = XOpenDisplay(NULL);
    }
    else{
        display = XOpenDisplay(dname.c_str());
    }
    
    if(display == NULL) {
        throw std::runtime_error("__get_display_dump__(std::size_t* w, std::size_t* h) : The XOpenDisplay() function failed:");  
    }

    const auto root = DefaultRootWindow(display);


    // Get window parameters (width and height):
    XWindowAttributes gwa;
    XGetWindowAttributes(display, root, &gwa);

    const auto width = gwa.width;
    const auto height = gwa.height;

    // set w, h arguments:
    *w = width;
    *h = height;

    // Make a screen dump:
    const auto image = XGetImage(display, root, 0, 0, width,height,AllPlanes, ZPixmap);
    if(image == NULL) {
        throw std::runtime_error("__get_display_dump__(std::size_t* w, std::size_t* h) : The XGetImage() function failed:");  
    }

    // Copy dump to unsigned char vector. Format: 24 bits { R(8b) G(8b) B(8b) } :
    pixelmap_t pmap(width * height * 3, 0);

    const auto red_mask = image->red_mask;
    const auto green_mask = image->green_mask;
    const auto blue_mask = image->blue_mask;

    for (unsigned int y = 0; y < height ; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            const auto pixel = XGetPixel(image, x, y);

            const auto blue =  static_cast<unsigned char>(pixel & blue_mask);
            const auto green = static_cast<unsigned char>((pixel & green_mask) >> 8);
            const auto red =   static_cast<unsigned char>((pixel & red_mask) >> 16);

            pmap[(x + y * width) * 3] = red;
            pmap[(x + y * width) * 3 + 1] = green;
            pmap[(x + y * width) * 3 + 2] = blue;
        }
    }

    return pmap;
}

} // namespace scap

#endif