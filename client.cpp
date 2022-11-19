#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "network/includes/srfc_connection.hpp"
#include "network/includes/srfc_request.hpp"
#include "network/includes/srfc_response.hpp"

#include "screencap/screencap.hpp"
#include "utilities/net_utils.hpp"
#include "utilities/array_deleter.hpp"
#include "utilities/filesystem_utils.hpp"

using namespace net;
using namespace scap;

using payload_t =  srfc_connection::payload_t;
using status_t = srfc_connection::status_t;
using params_t = srfc_connection::params_t;

std::string SCAP_DIR =  "scrshots/";
static std::string local_display = "";

static status_t PRINT_callback(const params_t&,payload_t,payload_t*,std::size_t*);
static status_t RUN_SCAP_callback(const params_t&,payload_t,payload_t*,std::size_t*);
static status_t STOP_SCAP_callback(const params_t&,payload_t,payload_t*,std::size_t*);
static status_t LIST_SCAP_callback(const params_t&,payload_t,payload_t*,std::size_t*);
static status_t GETFILE_SCAP_callback(const params_t&,payload_t,payload_t*,std::size_t*);

// flag to store the __scap_thread__ state (running / not started)
// used to stop screen capturing 
static std::atomic_bool run_scap_flag{false};

// Used for __scap_thread__ termination
static std::atomic_bool scap_finished_flag{true};
static std::condition_variable scap_finished_cv;
static std::mutex scap_finished_mutex;

void run_client(std::string address, std::string port, std::string display)
{
    std::cout << "Starting clinet" << std::endl << std::endl;

    unsigned int portval = 0;
    try{
        // std::stoul throws std::invalid_argument if no conversion could be performed or
        // std::out_of_range if the converted value would fall out of the range
        portval = std::stoul(port);
    }
    catch(const std::exception& e){
        std::string what = "Error while starting client: invalid port value: " + port + ".\n" + e.what();
        throw std::runtime_error(what);
    }
    
    // Set display variable
    // Determines the display to capture with scap
    local_display = display;

    // Create srfc_connection object with the "deferred" flag set.
    // It is done to prevent accepting requests and responses from a connected server
    // before all callbacks set. Throws std::logic_error if unsuccessful
    srfc_connection connection(portval, address, true);

    // Set client methods callbacks
    // Each rfc request (srfc_request) will be asynchronously passed to the appropriate 
    // handler (if found). The responce will be formed based on the  callback's 
    // return value (status_t) and returned payload (payload_t*).
    connection.add_method("PRINT", PRINT_callback);
    connection.add_method("RUN_SCAP", RUN_SCAP_callback);
    connection.add_method("STOP_SCAP", STOP_SCAP_callback);
    connection.add_method("LIST_SCAP", LIST_SCAP_callback);
    connection.add_method("GETFILE_SCAP", GETFILE_SCAP_callback);

    // Invoke deferred conneciton
    // Since now, connection starts listening for the incoming requests and responces
    connection.invoke_deferred();

    // Stop this thread to prevent connection from being destroyed
    // Wait for a stop condtition (connection closed)
    while(connection.is_connected()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << std::endl << "*** Connection closed. ***" << std::endl;
}


/******************************************************************/
/*                           Callbacks                            */
/******************************************************************/

static status_t PRINT_callback(
    const params_t& params,
    payload_t pld, 
    payload_t* rpld, 
    std::size_t* rpld_sz)
{
    std::cout << "PRINT srfc-request received" << std::endl;

    // get MESSAGE parameter value:
    std::string val;
    try{
        // throws if not found
        val = get_param(params, "MESSAGE");
    }
    catch(...) {
        return status_codes::invalid_arguments;
    }

    std::cout << val << std::endl;
    return status_codes::ok;
}

static void __scap_thread__(unsigned long interv);

static status_t RUN_SCAP_callback(
    const params_t& params,
    payload_t pld, 
    payload_t* rpld, 
    std::size_t* rpld_sz)
{
    std::cout << "RUN_SCAP srfc-request received" << std::endl;

    // get INTERVAL parameter value:
    unsigned long interv;
    try{
        // throws if not found
        const auto val = get_param(params, "INTERVAL");
        // throws std::invalid_argument, std::out_of_range if no conversion could be performed
        interv = std::stoul(val);
    }
    catch(...) {
        return status_codes::invalid_arguments;
    }

    // Check if not already running:
    if(run_scap_flag.load() == true) {
        std::string what = "Error: Screen capturer is already running";
        set_payload_msg(rpld, what, rpld_sz); // Sets rpld and rpld_sz;
        
        return status_codes::execution_error;
    }
    // thread is being terminated
    if(scap_finished_flag.load() == false) 
    {
        std::string what = "Error: Screen capturer is still terminating";
        set_payload_msg(rpld, what, rpld_sz); // Sets rpld and rpld_sz;
        
        return status_codes::execution_error;
    }

    // Start screen capture with given interval:
    run_scap_flag.store(true); // used to stop screen capture thread
    scap_finished_flag.store(false); // used to wait for screen capture termination
    std::thread(__scap_thread__, interv).detach();

    return status_codes::ok;
}

static status_t STOP_SCAP_callback(
    const params_t& params,
    payload_t pld, 
    payload_t* rpld, 
    std::size_t* rpld_sz)
{
    std::cout << "STOP_SCAP srfc-request received" << std::endl;

    // Check if has not been started:
    if(run_scap_flag.load() == false) {
        std::string what = "Error: Screen capturer has not been started";
        set_payload_msg(rpld, what, rpld_sz); // Sets rpld and rpld_sz;
        
        return status_codes::execution_error;
    }

    // Set the run_scap_flag to false to terminate screen capturing thread
    run_scap_flag.store(false);

    // wait for thread to finish
    std::unique_lock<std::mutex> ul(scap_finished_mutex);
    scap_finished_cv.wait(ul, []{return scap_finished_flag.load();});

    return status_codes::ok;
}

static status_t LIST_SCAP_callback(
    const params_t& params,
    payload_t pld, 
    payload_t* rpld, 
    std::size_t* rpld_sz)
{
    std::cout << "LIST_SCAP srfc-request received" << std::endl;

    // No arguments needed for this method
    try{
        // throws std::runtime_error on error
        const auto res = list_files(SCAP_DIR);
        std::string resMessage;
        for(const auto& v : res) {
            resMessage += v + "\n";
        }
        set_payload_msg(rpld, resMessage, rpld_sz); // Sets rpld and rpld_sz;
    }
    catch(...) {
        std::string what = "Error: can't get the filenames";
        set_payload_msg(rpld, what, rpld_sz); // Sets rpld and rpld_sz;
        
        return status_codes::execution_error;    
    }

    return status_codes::ok;
}

static status_t GETFILE_SCAP_callback(
    const params_t& params,
    payload_t pld, 
    payload_t* rpld, 
    std::size_t* rpld_sz)
{   
    std::cout << "GETFILE_SCAP srfc-request received" << std::endl;

    std::string filename;
    try{
        // throws if not found
        filename = get_param(params, "NAME");
    }
    catch(...) {
        return status_codes::invalid_arguments;
    }

    // Read file:
    std::ifstream ifs(SCAP_DIR + get_separator() + filename, std::ios::binary);
    if(!ifs.is_open()) {
        std::string what = "Error: cannot open file " + filename + ".";
        set_payload_msg(rpld, what, rpld_sz); // Sets rpld and rpld_sz;
    
        return status_codes::execution_error; 
    }

    std::vector<char> buffer(std::istreambuf_iterator<char>(ifs), {});
    set_payload_data(rpld, rpld_sz, buffer.data(), buffer.size()); // Sets rpld and rpld_sz;

    return status_codes::ok;
}
/******************************************************************/
/*                             Other                              */
/******************************************************************/

static void __scap_thread__(unsigned long interv) 
{
    while(true){
        try{
            Screencap::make_screenshot(Screencap::get_new_name(SCAP_DIR), local_display);
        }
        catch(...) {
            // terminate screen capture:
            run_scap_flag.store(false);
        }

        // terminate screen capture:
        if(!run_scap_flag.load()) {
            scap_finished_flag.store(true);
            scap_finished_cv.notify_all();
            return;
        }

        std::this_thread::sleep_for(std::chrono::seconds(interv));
    }
}
