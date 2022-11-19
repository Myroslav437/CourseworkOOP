#include <iostream>
#include <string>
#include <cstring>

static std::string address;
static std::string interface;
static std::string display;
static std::string port;
static bool listen = false;

static void parse_args(int argc, char *argv[]);
static void validate_globals();

// defined in server.cpp
extern void run_server(std::string port, std::string interface);
// defined in client.cpp
extern void run_client(std::string address, std::string port, std::string display);

int main(int argc, char *argv[]) 
{
    try{
        // parse arguments and set the corresponding global variables
        // throws std::runtime_error if fails
        parse_args(argc, argv);

        // validate global variables (address, interface, display, port, false).
        // do not check values - only confirm that they are correcty set.
        // throws std::runtime_error if fails
        validate_globals();

        // run client or server depending on whether the listen flag is set
        listen ? run_server(::port, ::interface) : 
                 run_client(::address, ::port, ::display);
    }
    catch(const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}

static void parse_args(int argc, char *argv[])
{
    if(argc < 2) {
        throw std::runtime_error("Passed too few arguments");
    }
    // parse for -l (is client if not found)
    if(!std::strcmp(argv[1], "-l")) {
        ::listen = true;
    }
    else {
        // than the first argument should be server address
        ::address = argv[1];
    }

    // parse for -i, -p -d
    for(int i = 2; i < argc; ++i) {
        // Check for the -i flag (interface):
        if(!std::strcmp(argv[i], "-i")) {
            if(i + 1 < argc && ::interface.empty()) {
                ::interface = argv[++i];
                continue;
            }
            else {
                std::string what = "Parsing error: invalid usage of -i; ";
                what += (::interface.empty() ? "Interface expected." : "Already setted with value " + ::interface + ".");
                throw std::runtime_error(what);
            }
        }
        // Check for the -p flag (port):
        else if(!std::strcmp(argv[i], "-p")) {
            if(i + 1 < argc && ::port.empty()) {
                ::port = argv[++i];
                continue;
            }
            else {
                std::string what = "Parsing error: invalid usage of -p; ";
                what += (::port.empty() ? "Port expected." : "Already setted with value " + ::port + ".");
                throw std::runtime_error(what);
            }
        }
        // Check for the -d flag (display):
        else if(!std::strcmp(argv[i], "-d")) {
            if(i + 1 < argc && ::display.empty()) {
                ::display = argv[++i];
                continue;
            }
            else {
                std::string what = "Parsing error: invalid usage of -d; ";
                what += (::display.empty() ? "Display expected." : "Already setted with value " + ::display + ".");
                throw std::runtime_error(what);
            }
        }
        else {
            std::string what = std::string("Parsing error: invalid argument ") + argv[i] + ".";
            throw std::runtime_error(what);    
        }
    }
}

static void validate_globals() 
{
    // validation if listen flag is set:
    if(listen && !address.empty()) {
        throw std::runtime_error("Validation error: -l flag is set but address is specified.");
    }
    if(listen && !display.empty()) {
        throw std::runtime_error("Validation error: -l flag is set but dispalay is specified.");    
    }
    if(listen && port.empty()) {
        throw std::runtime_error("Validation error: port not set.");    
    }

    // validation if is client:
    if(!listen && !interface.empty()) {
        throw std::runtime_error("Validation error: -l flag is not set but interface is specified.");     
    }
    if(!listen && address.empty()) {
        throw std::runtime_error("Validation error: address not set.");     
    }
    if(!listen && port.empty()) {
        throw std::runtime_error("Validation error: port not set.");     
    }
}




