#ifndef SRFC_LISTENER_HPP
#define SRFC_LISTENER_HPP

#include <string>
#include <functional>

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "srfc_connection.hpp"

namespace net 
{

class srfc_listener 
{
    using socket_t = srfc_connection::socket_t;
    using callback_t = srfc_connection::callback_t;
    using connection_callback_t = std::function<void(srfc_connection)>;
    
public:
    // make non-copyable:
    srfc_listener(const srfc_listener& other) = delete;
    srfc_listener& operator=(const srfc_listener& other) = delete;

    // move constructor & move assignment operator:
    // valid only on deffered objects
    srfc_listener(srfc_listener&& other);
    srfc_listener& operator=(srfc_listener&& other);

    // Default constructor & parameterized constructors & dtor:
    srfc_listener() = default;
    srfc_listener(socket_t socketFd, bool deferred = false);
    srfc_listener(unsigned int port, std::string address = "", bool deferred = false);
    ~srfc_listener();

    // 
    void    on_connection(connection_callback_t callback);

    // manipulating methods:
    void        add_method(std::string methodName, callback_t methodCallback);
    bool        remove_method(std::string methodName);
    callback_t  get_method(std::string methodName) const;
    bool        has_method(std::string methodName) const;

    // manipulating connection:
    void    listen(unsigned int port, std::string address, bool deferred = false);
    void    listen(socket_t bindedSockFd, bool deferred = false);
    void    invoke_deferred();
    bool    is_listening() const;
    void    shutdown();           
    void    reset();

protected:
    void    __listener_thread__();                         // listner_thread function 
    void    connection_handler(socket_t clientfd);

private:
    void        __bind__(unsigned int port, std::string address);   // platform-dependent implementataion
    socket_t    __accept__();                                       // platform-dependent implementataion
    void        __shutdown__();                                     // platform-dependent implementataion
    void        __close__();                                        // platform-dependent implementataion

private:
    socket_t socket_fd = 0;
    
    connection_callback_t connection_callback = [](const auto c){return;}; // do nothing
    std::unordered_map<std::string, callback_t> callback_map;
    
    std::mutex listener_cv_mutex;
    std::mutex idleable_cv_mutex;

    std::condition_variable listener_cv;
    std::condition_variable idleable_cv;

    std::atomic_bool binded {false};
    std::atomic_bool listening {false};
    std::atomic_bool idleable {true};
    std::atomic_bool terminate_listener {false};    // only setted in the constructor

    std::thread listener_thread;
};

} // namespace net 

#endif