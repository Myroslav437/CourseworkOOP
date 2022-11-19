#ifndef SRFC_CONNECTION_HPP
#define  SRFC_CONNECTION_HPP

#include <string>
#include <functional>
#include <vector>  
#include <unordered_map>

#include <future>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "srfc_request.hpp"
#include "srfc_response.hpp"

namespace net 
{

class srfc_connection 
{
public:
    using params_t = srfc_request::params_t;
    using payload_t = srfc_request::payload_t;
    using status_t = srfc_response::status_t;
    using serialized_t = srfc_request::serialized_t;
    using callback_t = std::function<status_t(const params_t&, payload_t, payload_t*, std::size_t*)>;
    using id_t = srfc_request::id_t;
    
    // For WinAPI: Even though sizeof(SOCKET) is 8, it's safe to cast it to int, because
    // the value constitutes an index in per-process table of limited size and not a real pointer.
    // https://stackoverflow.com/questions/1953639/is-it-safe-to-cast-socket-to-int-under-win64
    using socket_t = int;

    // Make non-copyable & non-movable:
    srfc_connection(const srfc_connection& other) = delete;
    srfc_connection& operator=(const srfc_connection& other) = delete;

    // Move operations:
    // Is ONLY VALID on deferred objects. 
    // If *this still has an associated running thread (i.e. joinable() == true), throws throw std::logic_error
    srfc_connection(srfc_connection&& other);
    srfc_connection& operator=(srfc_connection&& other);

    // Default constructor & parameterized constructors & dtor:
    srfc_connection() = default;
    srfc_connection(socket_t socketFd, bool deferred = false);
    srfc_connection(unsigned int port, std::string address, bool deferred = false);
    ~srfc_connection();

    // Manipulating the method map:
    void        add_method(std::string methodName, callback_t methodCallback);
    bool        remove_method(std::string methodName);
    callback_t  get_method(std::string methodName) const;
    bool        has_method(std::string methodName) const;

    // Sending requests and responses: 
    std::future<srfc_response>  send_request(const srfc_request& request);
    std::future<void>           send_response(const srfc_response& response);

    // Manipulating the connection:
    void    connect(unsigned int port, std::string address, bool deferred = false);
    void    connect(socket_t socketFd, bool deferred = false);
    void    invoke_deferred();
    bool    is_connected() const;
    void    shutdown();           
    void    reset();

protected:
    void            handle_request(srfc_request request); 
    void            handle_response(srfc_response response);             
    srfc_response   __send_request__(const srfc_request& request);
    void            __send_response__(const srfc_response& response);

private:
    // Platform-dependent methods:
    void              __listener__();                                         // platform-dependent implementation
    void              __connect__(unsigned int port, std::string address);    // platform-dependent implementation
    void              __send__(const void *buf, std::size_t len);             // platform-dependent implementation   
    std::vector<char> __receive__();                                          // platform-dependent implementation
    void              __shutdown__();                                         // platform-dependent implementation
    void              __close__();                                            // platform-dependent implementation

    // Manipulating the response queue:
    void            add_response(const srfc_response& response);
    bool            received_response(id_t request_id) const;
    srfc_response   read_response(id_t response_id);

    // Fields:

    std::unordered_map<std::string, callback_t> callback_map;
    std::vector<srfc_response> response_queue; // change conatiner to std::set
    socket_t socket_fd = 0;

    std::atomic_bool connected{false};     // setted true ONLY in the connect() function, setted false ONLY in the shutdown()           
    std::atomic_bool terminate_listener{false}; // setted ONLY in the destructor;
    std::atomic_bool idleable{true};
    
    mutable std::mutex queue_mutex;
    mutable std::mutex listener_cv_mutex;
    mutable std::mutex idleable_cv_mutex;    
    mutable std::mutex response_cv_mutex;
    mutable std::condition_variable listener_cv;
    mutable std::condition_variable response_cv;
    mutable std::condition_variable idleable_cv;   

    std::thread listener;
}; // class srfc_connection

} // namespace net 

#endif