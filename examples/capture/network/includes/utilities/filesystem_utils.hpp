#include <experimental/filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

inline std::vector<std::string> list_files(std::string dir) 
{
    namespace fs = std::experimental::filesystem;

    std::vector<std::string> res;

    try{     
        // iterate through every element in the folder
        std::for_each(
            fs::directory_iterator(dir),
            fs::directory_iterator(),
            [&](const fs::directory_entry& dir_entry) 
            {
                if(fs::is_regular_file(dir_entry.status())) {
                    const fs::path fname = dir_entry.path().filename();
                    res.push_back(fname.string());
                }
            } 
        );
    }
    catch(...) {
        std::string what = "list_files(std::string dir): Error while reading " + dir + ".";
        throw std::runtime_error(what);
    }

    return res;
}

inline std::string get_separator()
{
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    return "\\";
#else
    return "/";
#endif
}

inline void save_to_file(std::string fname, const char* data, std::size_t size) 
{
    std::ofstream ofs(fname, std::ios::out | std::ios::binary);
    if(!ofs.is_open()) {
        throw std::runtime_error("Can't open the file " + fname + ".");
    }

    ofs.write(data, size);
    ofs.close();
}

inline void create_folder(std::string dirname)
{
    namespace fs = std::experimental::filesystem;

    auto created_new_directory = fs::create_directory(dirname);
    if (!created_new_directory) {
        // Either creation failed or the directory was already present.
        throw std::runtime_error("create_folder(std::string dirname): cant create folder " + dirname + ".");
    }
    
}