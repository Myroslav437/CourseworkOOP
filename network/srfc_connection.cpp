#include "includes/srfc_connection.hpp"

#include <algorithm>
#include <stdexcept>

#include "../utilities/alg.hpp"
#include "../utilities/net_utils.hpp"
#include "../utilities/array_deleter.hpp"

namespace net
{

srfc_connection::srfc_connection(unsigned int port, std::string address, bool deferred)
{
    connect(port, address, deferred);
}

srfc_connection::srfc_connection(socket_t socketFd, bool deferred)
{
    connect(socketFd, deferred);
}

srfc_connection::srfc_connection(srfc_connection&& other)
{
    *this = std::move(other);
}

srfc_connection& srfc_connection::operator=(srfc_connection&& other)
{
    if(this->listener.joinable() == true) {
        throw std::logic_error("operator=(srfc_connection&& other): is not deferred");
    }
    if(other.listener.joinable() == true) {
        throw std::logic_error("operator=(srfc_connection&& other): is not deferred");        
    }

    callback_map = std::move(other.callback_map);
    other.callback_map.clear();

    response_queue = std::move(other.response_queue);
    other.response_queue.clear();

    socket_fd = other.socket_fd;
    other.socket_fd = 0;

    connected.store(other.connected.load());
    other.connected.store(false);

    terminate_listener.store(other.terminate_listener.load());
    other.terminate_listener.store(false);

    idleable.store(other.idleable.load());
    other.idleable.store(true);

    listener = std::move(other.listener);

    return *this;
}

void srfc_connection::add_method(std::string methodName, callback_t methodCallback)
{
    callback_map[methodName] = methodCallback;
}

bool srfc_connection::remove_method(std::string methodName)
{
    // std::unordered_map::erase returns number of elements removed (0 or 1)
    auto res = callback_map.erase(methodName);
    return res == 1 ? true : false; 
}

srfc_connection::callback_t 
srfc_connection::get_method(std::string methodName) const
{
    // If no such element exists, an exception of type std::out_of_range is thrown
    return callback_map.at(methodName);
}

bool srfc_connection::has_method(std::string methodName) const
{
    return callback_map.find(methodName) != callback_map.end();
}

std::future<srfc_response> 
srfc_connection::send_request(const srfc_request& request)
{
    if(connected.load() == false) {
        throw std::logic_error("send_request(const srfc_request& request): not connected");
    }
    return std::async(&srfc_connection::__send_request__, this, std::ref(request));
}

std::future<void> 
srfc_connection::send_response(const srfc_response& response) 
{
    if(connected.load() == false) {
        throw std::logic_error("send_response(const srfc_response& response): not connected");
    }
    return std::async(&srfc_connection::__send_response__, this, response);
}

void srfc_connection::connect(unsigned int port, std::string address, bool deferred)
{
    if(connected.load() == true) {
        throw std::logic_error("connect(unsigned int port, std::string address): is already connected"); 
    }

    __connect__(port, address); // platform-dependent implementation'
                                // sets socket_t socket_fd
                                // sets std::atomic_bool connected

    if(!deferred) {
        // listener has not been started yet:
        if(listener.joinable() == false) {
            listener = std::thread(&srfc_connection::__listener__, this);
        }
        // listener is idled:
        else {
            listener_cv.notify_one();
        }
    }
}

void srfc_connection::connect(socket_t socketFd, bool deferred)
{
    if(connected.load() == true) {
        throw std::logic_error("connect(socket_t socketFd): is already connected"); 
    }

    this->socket_fd = socketFd;
    connected.store(true);

    if(!deferred) {
        // listener has not been started yet:
        if(listener.joinable() == false) {
            listener = std::thread(&srfc_connection::__listener__, this);
        }
        // listener is idled:
        else {
            listener_cv.notify_one();
        }
    }
}

void srfc_connection::invoke_deferred()
{
    // listener has not been started yet:
    if(listener.joinable() == false) {
        listener = std::thread(&srfc_connection::__listener__, this);
    }
    // listener is idled:
    else {
        listener_cv.notify_one();
    }
}

bool srfc_connection::is_connected() const
{
    return connected.load();
}

// shutdown connection, close socket, idle receive_thread, clear the response_queue
void srfc_connection::shutdown() 
{
    if(connected.load() == false) {
        throw std::logic_error("shutdown(): is not connected");
    }

    connected.store(false);
    listener_cv.notify_one();

    // awakes blocking operations
    try{ 
        // try to shutdown
        // throws if connection was closed from another host
        __shutdown__();
    }
    catch(...){}

    __close__();
    socket_fd = 0;

    if(listener.joinable()) {
        // wait for listener to idle on listener_cv
        std::unique_lock<std::mutex> ul(idleable_cv_mutex);
        idleable_cv.wait(ul, [this]{return this->idleable.load();});
    }

    response_cv.notify_all();

    std::lock_guard<std::mutex> lg(queue_mutex);
    response_queue.clear();
}

// if thread was not started?
void srfc_connection::reset()
{
    // connected can be false ONLY if the shutdown() was previously called OR connect() was not called yet
    if(connected.load() == true) {
        shutdown();
    }
    callback_map.clear();
}

// if thread was not started?
srfc_connection::~srfc_connection()
{
    reset();

    // Terminate the terminate_listener thread if it was started:
    if(listener.joinable()) {
        terminate_listener.store(true);
        listener_cv.notify_one();
        listener.join();
    }
}

void srfc_connection::handle_request(srfc_request request)
{
    auto rid = request.getRequestId();
    auto response = srfc_response(rid);

    // No requested method found:
    if(!has_method(request.getMethod())) {
        response.setStatusCode(status_codes::unknown_method);
    }

    // Method found:
    else {
        payload_t respPld;
        std::size_t respPldSz = 0;
        
        // Execute method: 
        auto callback = get_method(request.getMethod());
        status_t res;
        try {
            res = callback(
                request.getParams(),
                request.getPayload(),
                &respPld,
                &respPldSz
            );
        }
        catch(...) {
            res = status_codes::unhandled_exception;
            respPldSz = 0;
            respPld.reset();
        }

        // Add results to response:
        response.setStatusCode(res);
        response.setPayload(respPld, respPldSz);
    }
    
    // Send response:
    send_response(response);
}

void srfc_connection::handle_response(srfc_response response)
{
    add_response(response);
    response_cv.notify_all(); // notify __send_request__ threads about the new response
}

srfc_response srfc_connection::__send_request__(const srfc_request& request)
{
    auto requestId = request.getRequestId();
    std::size_t srdSz = 0;
    auto srd = request.serialize(&srdSz);
    
    // try to send
    try {
        __send__(static_cast<const void*>(srd.get()), srdSz);
    }
    catch(...){
        return srfc_response(requestId, status_codes::connection_error);      
    }

    // Wait for response:
    std::unique_lock<std::mutex> ul(response_cv_mutex);
    response_cv.wait(ul, [requestId, this] {  // change for wait_for?
        return !connected.load() || this->received_response(requestId);
    });

    if(!connected.load()) {
        return srfc_response(requestId, status_codes::connection_error);
    }

    // Response received:
    try {
        auto res = read_response(requestId);
        return res;
    }
    catch(...) {
        return srfc_response(requestId, status_codes::connection_error);    
    }
}

void srfc_connection::__send_response__(const srfc_response& response)
{
    std::size_t srdSz = 0;
    const auto srd = response.serialize(&srdSz);
    __send__(static_cast<const void*>(srd.get()), srdSz);
}

void srfc_connection::add_response(const srfc_response& response)
{
    std::lock_guard<std::mutex> lg(queue_mutex);
    response_queue.emplace_back(std::move(response));   
}

bool srfc_connection::received_response(id_t request_id) const
{
    std::lock_guard<std::mutex> lg(queue_mutex);

    auto it = std::find_if(response_queue.begin(), response_queue.end(), [request_id](const auto& resp){
        return resp.getRequestId() == request_id;
    });

    // no element found:
    return it != response_queue.end();
}

srfc_response srfc_connection::read_response(id_t response_id)
{
    std::lock_guard<std::mutex> lg(queue_mutex);

    auto it = std::find_if(response_queue.begin(), response_queue.end(), [response_id](const auto& resp){
        return resp.getRequestId() == response_id;
    });

    // throw std::logic_error if nothing found:
    if(it == response_queue.end()) {
        throw std::logic_error("read_response(id_t response_id): no response with id = " + 
                               std::to_string(response_id) + " found");
    }

    // copy found response:
    const auto result = *it;

    // remove this response from queue:
    response_queue.erase(it);

    return result;
}

void srfc_connection::__listener__()
{
    std::vector<char> receivedData;  
    receivedData.reserve(2048); // reserve 2KB (arbitrary-chosen size);

    // main listener loop:
    while(true) {
        // set idleable flag and notify. Shutdown() method now can continue execution:
        idleable.store(true);
        idleable_cv.notify_all();
        
        // get previous connection handle; used to clear receivedData after connection change
        const auto prev_connection = this->socket_fd;

        // clear receivedData if disconnected:
        if(!connected.load()) {
            receivedData.clear();
        }

        // idle if not connected:
        std::unique_lock<std::mutex> ul(listener_cv_mutex);
        listener_cv.wait(ul, [this] {
            return connected.load() || terminate_listener.load();
        });

        // finish thread if terminate_listener flag set:
        if(terminate_listener.load()) {
            return;
        }

        // remove the idleable_cv flag. Shutdown() method should wait for the end of this iteration:
        idleable.store(false);

        // clear the receivedData message if connection changed during idle:
        if(prev_connection != this->socket_fd) {
            receivedData.clear();
        }

        // read data:
        std::vector<char> res; // container for the reseived chunk
        try{
            res = __receive__();
        }   
        catch(...){
            receivedData.clear();
            continue;
        }

        // Connection was terminated:
        if(res.empty()) {
            receivedData.clear();
            idleable.store(true);
            this->shutdown();
            continue;
        }

        // copy received chunk into the whole message
        receivedData.insert(receivedData.end(), res.begin(), res.end());
        
        // check whether message contains preamble. If not go to the next iteration
        if(receivedData.size() < 32) { 
            continue;
        }
        
        // get desired message size:
        std::size_t message_size = 0;
        try{
            message_size = get_size_from_preamble(receivedData.data());
        }
        catch(...) {
            // Invalid preamble
            receivedData.clear();
            continue;
        }
        
        // go to a new iteration if entire message received:
        if(receivedData.size() < message_size){
            continue;
        }

        // if entire message reseived:

        // create serialized_t variable and move the received message with extraction:
        serialized_t serl(new char[message_size], array_deleter<char>());
        memcpy(serl.get(), receivedData.data(), message_size);
        receivedData.erase(receivedData.begin(), receivedData.begin() + message_size);

        // validate message and pass to handlers (each as a new detached thread):
        if(is_valid_message(serl, message_size)){
            // get message type:
            const auto type = extract_type(serl, message_size);

            if(type == "REQ") {
                srfc_request tmp(serl, message_size);
                std::thread([this, tmp]{handle_request(std::move(tmp));}).detach();
            }
            else if(type == "RES") {
                srfc_response tmp(serl, message_size);
                std::thread([this, tmp]{handle_response(std::move(tmp));}).detach();
            }
        }
    }
}

} // namespace net