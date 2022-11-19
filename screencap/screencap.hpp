#ifndef SCREENCAP_HPP
#define SCREENCAP_HPP

#include <string>
#include <vector>
#include <atomic>
#include <mutex>

namespace scap 
{

class Screencap
{
    using pixelmap_t = std::vector<unsigned char>;

public:
    static void make_screenshot(const std::string& savePath, const std::string& display_name);
    static std::string get_new_name(std::string initPath = "");

protected:
    static void save_img(const pixelmap_t& pmap, const std::string& path);
    static void convert_to_ppm(pixelmap_t& map, std::size_t w, std::size_t h);
    static pixelmap_t __get_display_dump__(std::size_t* w, std::size_t* h, const std::string& dname); // platform-dependent implementation

private:
    static std::atomic_size_t counter;
    static std::string name_base;
    static std::mutex name_base_mutex;
};

} // namespace scap

#endif