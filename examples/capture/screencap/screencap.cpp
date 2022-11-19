#include "screencap.hpp"

#include <ctime>
#include <fstream>

//fix
#include "../network/includes/utilities/alg.hpp"
#include "../network/includes/utilities/filesystem_utils.hpp"

namespace scap
{

std::atomic_size_t Screencap::counter {1};
std::string Screencap::name_base  = "";
std::mutex Screencap::name_base_mutex{};
    
void Screencap::make_screenshot(const std::string& savePath, const std::string& dname)
{   
    // get pixelmap of the dumped screen
    std::size_t w, h;
    auto pmap = __get_display_dump__(&w, &h, dname);
    
    // convert received pixelmap to the Netpbm P6 format
    convert_to_ppm(pmap, w, h);

    // save image:
    save_img(pmap, savePath);
}

void Screencap::convert_to_ppm(pixelmap_t& map, std::size_t w, std::size_t h) 
{
    // compose Netpbm P6 header:
    std::string header = "P6\n";
    header += std::to_string(w) + " ";
    header += std::to_string(h) + "\n";
    header += "255\n";

    // insert header into the beginning of pixelmap
    map.insert(map.begin(), header.begin(), header.end());
}

void Screencap::save_img(const pixelmap_t& pmap, const std::string& path)
{
    bool tried_flag = false;

    std::ofstream fs(path, std::ios::binary);
    while(!fs.is_open()) {
        // Try to create folder and open again:
        try {
            if(tried_flag){
                throw std::exception();
            }
            create_folder(path.substr(0, path.find_last_of("/\\")));
            fs.open(path, std::ios::binary);
            tried_flag = true;
        }
        catch(...){
            throw std::runtime_error("Can't open the file " + path);
        }
    }
    // reinterpret_cast from <const unsigned char*> to <const char*>
    fs.write(reinterpret_cast<const char*>(pmap.data()), pmap.size());
    fs.close();
}

std::string Screencap::get_new_name(std::string initPath)
{
    std::time_t t = std::time(0);
    std::tm* now = std::localtime(&t);

    std::string fname = 
        std::to_string(now->tm_mday) + "." +
        std::to_string(now->tm_mon + 1) + "." +
        std::to_string(now->tm_year + 1900);
   
    if(name_base.empty()) {
        name_base = fname;
    }
    else if(name_base != fname) {
        // data changed
        name_base = fname;
        counter = 1;
    }

    while(true) {
        // test if file exists and move counter if needed
        std::string name = initPath + name_base + "_" + std::to_string(counter.load()) + ".ppm";
        std::ifstream test(name, std::ios::binary);

        if(test.is_open()){
            // already exists:
            test.close();
            counter += 1;
            continue;
        }
        else {
            counter += 1;
            return name;
        }
    }
}

} // namespace scap