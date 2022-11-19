#include <iostream>
#include <string>

#include "network/includes/srfc_listener.hpp"
#include "network/includes/utilities/net_utils.hpp"

using namespace std;
using namespace net;

// server port and address
const unsigned int port = 5555;
const std::string address("127.0.0.1");

// rpc for printing message into console:
srfc_connection::status_t print_rpc (
    const srfc_connection::params_t& in_params, 
    srfc_connection::payload_t in_payload, 
    srfc_connection::payload_t* out_payload, 
    size_t* out_paysz) 
{
    // set no payload in response:
    *out_payload = nullptr;
    *out_paysz = 0;

    string message;
    try{
        message = get_param(in_params, "MESSAGE");  // throws if not found
    }
    catch(...) {
        return status_codes::invalid_arguments;
    }

    // print:
    cout << "CLIENT: " << message << endl;

    // ok status code:
    return status_codes::ok;
}

void on_connection_callback(srfc_connection client) {
    client.invoke_deferred();
    while (true) {
        // get user input
       string message;
       getline(cin, message);
       
        // create RPC-request
        srfc_request rq("PRINT");
        rq.addParam("MESSAGE", message);
        
        // send RPC-request
        client.send_request(rq);
    }
}

int main() 
{
    // create client 
    srfc_listener ser;

    // add RPC
    ser.add_method("PRINT", print_rpc);
    ser.on_connection(&on_connection_callback);

    // connect to the server
    try{
        ser.listen(port, address);
    }
    catch(const std::exception& e) {
        cout << "Cannot start server" << endl;
        cout << e.what() << endl;
        return 1;
    }

    cout << "Server started at " << address << ": "<< port << endl;

    while (true); // wait

    return 0;
}