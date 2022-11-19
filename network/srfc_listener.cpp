#include "includes/srfc_listener.hpp"

namespace net 
{

srfc_listener::srfc_listener(socket_t socketFd, bool deferred)
{
    listen(socketFd, deferred);
}

srfc_listener::srfc_listener(unsigned int port, std::string address, bool deferred)
{
    listen(port, address, deferred);
}

srfc_listener::srfc_listener(srfc_listener&& other)
{
    *this = std::move(other);
}

srfc_listener& srfc_listener::operator=(srfc_listener&& other)
{
    if(this->listener_thread.joinable() == true) {
        throw std::logic_error("operator=(srfc_listener&& other): is not deferred");
    }
    if(other.listener_thread.joinable() == true) {
        throw std::logic_error("operator=(srfc_listener&& other): is not deferred");        
    }

    callback_map = std::move(other.callback_map);
    other.callback_map.clear();

    connection_callback = std::move(other.connection_callback);
    other.connection_callback = [](const auto c){return;}; // do nothing

    socket_fd = other.socket_fd;
    other.socket_fd = 0;

    listening.store(other.listening.load());
    other.listening.store(false);

    terminate_listener.store(other.terminate_listener.load());
    other.terminate_listener.store(false);

    binded.store(other.binded.load());
    other.binded.store(false);

    idleable.store(other.idleable.load());
    other.idleable.store(true);

    listener_thread = std::move(other.listener_thread);

    return *this;
}

void srfc_listener::on_connection(connection_callback_t callback)
{
    connection_callback = callback;
}

void srfc_listener::add_method(std::string methodName, callback_t methodCallback)
{
    callback_map[methodName] = methodCallback;
}

bool srfc_listener::remove_method(std::string methodName)
{
    // std::unordered_map::erase returns number of elements removed (0 or 1)
    auto res = callback_map.erase(methodName);
    return res == 1 ? true : false; 
}

srfc_listener::callback_t 
srfc_listener::get_method(std::string methodName) const
{
    // If no such element exists, an exception of type std::out_of_range is thrown
    return callback_map.at(methodName);
}

bool srfc_listener::has_method(std::string methodName) const
{
    return callback_map.find(methodName) != callback_map.end();
}

void srfc_listener::listen(unsigned int port, std::string interface, bool deferred)
{
    if(listening.load() == true) {
        throw std::logic_error("listen(unsigned int port, std::string address): is already listening.\n"
                               "Call shutdown() beforehand to change the listening address"); 
    }
    if(binded.load() == true) {
        throw std::logic_error("listen(unsigned int port, std::string address): is already binded.\n"
                               "Call shutdown() beforehand to change the listening address"); 
    }

    __bind__(port, interface);    // sets socket_t socket_fd
                                // sets std::atomic_bool binded;

    if(!deferred) {
        listening.store(true);

        // listner_thread has not been started yet:
        if(listener_thread.joinable() == false) {
            listener_thread = std::thread(&srfc_listener::__listener_thread__, this);
        }
        // listener is idled:
        else {
            listener_cv.notify_one();
        }
    }
}

void srfc_listener::listen(socket_t bindedSockFd, bool deferred)
{
    if(listening.load() == true) {
        throw std::logic_error("listen(socket_t socketFd): is already listening.\n"
                               "Call shutdown() beforehand to change the listening address"); 
    }
    if(binded.load() == true) {
        throw std::logic_error("listen(socket_t socketFd): is already binded.\n"
                               "Call shutdown() beforehand to change the listening address"); 
    }

    this->socket_fd = bindedSockFd;
    binded.store(true);

    if(!deferred) {
        listening.store(true);

        // listner_thread has not been started yet:
        if(listener_thread.joinable() == false) {
            listener_thread = std::thread(&srfc_listener::__listener_thread__, this);
        }
        // listener is idled:
        else {
            listener_cv.notify_one();
        }
    }
}

void srfc_listener::invoke_deferred()
{
    if(listening.load() == true) {
        throw std::logic_error("invoke_deferred(): is already listening.\n"
                                "Call shutdown() and listen() beforehand"); 
    }
    if(binded.load() == false) {
        throw std::logic_error("invoke_deferred(): is not binded.\n"
                               "Call listen() beforehand"); 
    }    

    listening.store(true);

    // listner_thread has not been started yet:
    if(listener_thread.joinable() == false) {
        listener_thread = std::thread(&srfc_listener::__listener_thread__, this);
    }
    // listener is idled:
    else {
        listener_cv.notify_one();
    }
}

bool srfc_listener::is_listening() const
{
    return listening.load();
}

void srfc_listener::shutdown()
{
    if(binded.load() == false) {
        throw std::logic_error("shutdown(): is not binded");
    }

    binded.store(false);

    // awake the listener_thread and idle on the listener_cv:
    listening.store(false);
    listener_cv.notify_one();

    __shutdown__(); 
    __close__();

    // wait for listener to idle on listener_cv
    if(listener_thread.joinable()) {
        std::unique_lock<std::mutex> ul(idleable_cv_mutex);
        idleable_cv.wait(ul, [this]{return this->idleable.load();});
    }
}   

void srfc_listener::reset()
{
    // binded can be false ONLY if the shutdown() was previously called OR listen was not called yet

    if(binded.load() == true) {
        shutdown();
    }
    callback_map.clear();
    connection_callback = [](const auto&){return;}; // do nothing
}

srfc_listener::~srfc_listener()
{
    reset();

    // Terminate the terminate_listener thread if it was started:
    if(listener_thread.joinable()) {
        terminate_listener.store(true);
        listener_cv.notify_one();
        listener_thread.join();
    }
}

void srfc_listener::__listener_thread__()
{
    while (true) {
        idleable.store(true);
        idleable_cv.notify_all();

        std::unique_lock<std::mutex> ul(listener_cv_mutex);
        listener_cv.wait(ul, [this]{
            return this->listening.load() || this->terminate_listener.load();
        });

        if(terminate_listener.load() == true) {
            return;
        }

        idleable.store(false);

        const auto client_fd = __accept__();
        std::thread([this, client_fd]{this->connection_handler(client_fd);}).detach();
    }
}

void srfc_listener::connection_handler(socket_t clientfd)
{
    // create DEFFERED connection:
    srfc_connection tmp(clientfd, true);

    // add methods:
    for(const auto& p : callback_map) {
        tmp.add_method(p.first, p.second);
    }
    // pass DEFFERED connection:
    connection_callback(std::move(tmp));
}

} // namespace net 