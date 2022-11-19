#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <chrono>

#include <sstream>
#include <iomanip>

#include "network/includes/srfc_connection.hpp"
#include "network/includes/srfc_listener.hpp"
#include "network/includes/srfc_request.hpp"
#include "network/includes/srfc_response.hpp"

#include "screencap/screencap.hpp"
#include "utilities/net_utils.hpp"
#include "utilities/filesystem_utils.hpp"

using namespace net;

using payload_t =  srfc_connection::payload_t;
using status_t = srfc_connection::status_t;
using params_t = srfc_connection::params_t;

static void on_connection_callback(srfc_connection con);
static status_t PRINT_callback(const params_t& params, payload_t pld, payload_t* rpld, std::size_t* rpld_sz);
static void run_interactive_console(srfc_connection con);

void run_server(std::string port, std::string interface)
{   
    std::cout << "Starting server" << std::endl << std::endl;
    std::cout << "WARNING: on_connection_callback() uses interactive mode" << std::endl;
    
    unsigned int portval = 0;
    try{
        // std::stoul throws std::invalid_argument if no conversion could be performed or
        // std::out_of_range if the converted value would fall out of the range
        portval = std::stoul(port);
    }
    catch(const std::exception& e){
        std::string what = "Error while starting listener: invalid port value: " + port + ".\n" + e.what();
        throw std::runtime_error(what);
    }

    // Create listener object with the "deferred" flag set.
    // It is done to prevent accepting connections before all callbacks set
    // throws std::logic_error if unsuccessful
    srfc_listener listener(portval, interface, true);

    // Set callback on a new connection accepted.
    // Will be called asynchronously for each connection;
    listener.on_connection(on_connection_callback);

    // Set listener methods callbacks
    // Each rfc request (srfc_request) will be asynchronously passed to the appropriate 
    // handler (if found). The responce will be formed based on the  callback's 
    // return value (status_t) and returned payload (payload_t*).
    listener.add_method("PRINT", PRINT_callback);

    // Start listening for the incoming connections
    listener.invoke_deferred();

    std::cout << "Waiting for connections..." << std::endl << std::endl;
    
    // Stop this thread to prevent connection from being destroyed
    // Wait for a stop condtition (currnetly absent)
    while (true);
}


/******************************************************************/
/*                           Callbacks                            */
/******************************************************************/

static void on_connection_callback(srfc_connection con) 
{
    // This callback will be called asynchronously for each connection;

    std::cout << "*** New connection ***" << std::endl ;
    std::cout << "Starting interactive console" << std::endl ;

    // Run server interactive console;
    // The capabilities of interactcive mode are significantly cut off. I.e, 
    // it can't work with the binary data and non-ASCII-7 encodings. 
    // Also it is not possible to work with serveral connection in this mode. 
    // Additionally, method parameterscan't contain the newline symbol.
    // Hence, it should be used only for debugging and demonstrating purposes.
    //
    // To use all capabilities, use the implemented srfc functionality.
    run_interactive_console(std::move(con));
}

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

/******************************************************************/
/*                             Other                              */
/******************************************************************/

static void cout_request(const srfc_request& request)
{
    const auto time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );
    std::cout << std::put_time(std::localtime(&time), "%Y-%m-%d %X");

    std::cout << " Request sent:" << std::endl;
    std::cout << "|-------------------------------------------|" << std::endl;
    std::cout << request.to_string() << std::endl;
    std::cout << "|-------------------------------------------|" << std::endl;
    std::cout << std::endl;
}

/*Duplicated code (print_request). Will be fixed later*/
static void cout_responce(const srfc_response& response)
{
    const auto time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );
    std::cout << std::put_time(std::localtime(&time), "%Y-%m-%d %X");

    std::cout << " Response received:" << std::endl;
    std::cout << "|-------------------------------------------|" << std::endl;
    std::cout << response.to_string() << std::endl;
    std::cout << "|-------------------------------------------|" << std::endl;
    std::cout << std::endl;
}

static srfc_request make_requst_from_console() 
{
    srfc_request rq;
    std::string buf;

    /*-------------------------*/
    /*       Add Method:       */
    /*-------------------------*/

    std::cout << "Enter method: "  << std::flush;
    std::getline(std::cin, buf);

    if(buf.empty()) {
        std::cout << "Invalid method name. Terminating interactive request builder." << std::endl;
        throw std::invalid_argument("Invalid method name");
    }

    rq.setMethod(buf);

    /*-------------------------*/
    /*     Add parameters:     */
    /*-------------------------*/
    for(int i = 1 ; ;++i) {
        std::cout << "Set Param"+ std::to_string(i) + ". Format: <name>: <value>" << std::endl;
        std::cout << " (enter to skip): "  << std::flush;

        std::getline(std::cin, buf);

        // all parameters entered:
        if(buf.empty()) {
            break;
        }

        typename params_t::value_type p;
        try{
            p = separate_param_val(buf); 
        }
        catch(...){
            std::cout << "Invalid prarmeter. Terminating interactive request builder." << std::endl;
            throw std::invalid_argument("Invalid parameter value");
        }

        rq.addParam(p.first, p.second);
    }

    /*-------------------------*/
    /*      Add Payload:       */
    /*-------------------------*/
    std::cout << "Include payload? (Y, N): " << std::flush;

    std::getline(std::cin, buf);

    // if yes
    if(!buf.empty() || buf[0] == 'Y' || buf[0] == 'y') {
        std::cout << "Enter payload (input <END> to stop):" << std::endl;
        std::vector<char> pld_buffer;

        while(true) {
            std::getline(std::cin, buf);
            if(buf == "<END>") {
                break;
            }

            pld_buffer.insert(pld_buffer.end(), buf.begin(), buf.end());
        }  

        // Set payload:
        payload_t pld;
        std::size_t pld_sz;
        set_payload_data(&pld, &pld_sz, pld_buffer.data(), pld_buffer.size());

        rq.setPayload(pld, pld_sz);
    }

    return rq;
}

static void run_interactive_console(srfc_connection con)
{
    std::cout << "|----------------------------------------------------------------------|" << std::endl;
    std::cout <<  
                 "\t\tInteracitve console v1\n\n"
                 " The capabilities of the interactcive mode are majorly reduced. I.e \n"
                 "   it can't work with the binary data and non-ASCII-7 encodings.\n"
                 " It is not possible to work with serveral connection in this mode.\n"
                 " Additionally, method parameters can't contain the newline symbol.\n"
                 " This mode should be used for debugging and demonstrating purposes only."
        << std::endl;
    std::cout << "|------------------------------------------------------------------------|" 
        << std::endl << std::endl << std::endl;


    // srfc_connection object is passed in deferred state, since 
    // move operation for srfc_connection is ONLY valid on deferred objects.
    // Invoke deferred srfc_connection object to start receive requests and responses
    con.invoke_deferred();

    // Interactive server loop
    while (con.is_connected()) {
        std::cout << std::endl << std::endl;
        std::cout << "Press enter to make a new request" << std::flush;
        std::string tmp; 
        std::getline(std::cin, tmp);
        
        // Get request:
        srfc_request rq;
        try{
            // throws on error
            rq = make_requst_from_console();
        }
        catch(...) {
            continue;
        }

        std::cout << "Sending request..." << std::endl;

        if(!con.is_connected()){
            break;
        } 
        auto res = con.send_request(rq);

        std::cout << "Waiting for response..." << std::endl;
        auto resp = res.get();

        std::cout << std::endl;
        cout_responce(resp);

        // check for payload
        payload_t pld;
        std::size_t pld_sz;
        pld = resp.getPayload(&pld_sz);

        if(pld_sz == 0) {
            continue;
        }
    
        // Payload present
        std::cout << "Response constains payload. Save it to a file? (Y, N) " << std::flush;
        std::string ans;
        std::getline(std::cin, ans);

        // If save payload:
        if(!ans.empty() || ans[0] == 'Y' || ans[0] == 'y') {
            while(true){
                std::cout << "Enter output path (Press Enter to discard): " << std::flush;
                std::string ans;
                std::getline(std::cin, ans);
                if(ans.empty()) {
                    break;
                }

                try{
                    // throws if fails:
                    save_to_file(ans, pld.get(), pld_sz);
                    break;
                }
                catch(const std::exception& e){
                    std::cout << std::endl << std::string("Error while saving to file: ") + e.what() << std::endl;
                    std::cout << std::endl;
                    continue;
                }
            } // while(true)
        } // if(!ans.empty() || ans[0] == 'Y' || ans[0] == 'y')
    }

    std::cout << std::endl  << "Clent disconnected." << std::endl << std::endl;;
    std::cout << "Waiting for a new connection." << std::endl;
}