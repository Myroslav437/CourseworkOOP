#include <iostream>
#include <string>

#include "network/includes/srfc_connection.hpp"
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
    cout << "Server: " << message << endl;

    // ok status code:
    return status_codes::ok;
}

int main() 
{
    // create client 
    srfc_connection cli;

    // add RPC
    cli.add_method("PRINT", print_rpc);

    // connect to the server
    try{
        cli.connect(port, address);
    }
    catch(...) {
        cout << "Cannot connect to the server" << endl;
        return 1;
    }


    cout << "Connected to the server at address: " << address << ": "<< port << endl;

    // chatting loop:
    while(true) {
        // get user input
        string message;
        getline(cin, message);

        // create request
        srfc_request rq("PRINT");
        rq.addParam("MESSAGE", message);

        // send request:
        cli.send_request(rq);
    }

    return 0;
}