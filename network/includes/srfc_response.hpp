#ifndef SRFC_RESPONSE_HPP
#define SRFC_RESPONSE_HPP

#include <memory>
#include <string>

namespace net
{

class status_codes {
public:
    using status_t = unsigned long;

    static const status_t none = 0;

    // Success status codes
    static const status_t ok = 200;
    static const status_t non_auth_information = 203;
    static const status_t no_content = 204;

    // Request errors:
    static const status_t bad_request = 400;
    static const status_t unauthorized = 401;
    static const status_t not_implemented = 402;
    static const status_t forbidden = 403;
    static const status_t unknown_method = 404;
    static const status_t conflict = 405;

    // Method execution errors:
    static const status_t execution_error = 500;
    static const status_t unhandled_exception = 501;
    static const status_t invalid_arguments = 502;
    static const status_t connection_error = 503;
    static const status_t response_timeout = 504;
};

class srfc_response 
{
public:
    using id_t = unsigned long;
    using payload_t = std::shared_ptr<char>;
    using serialized_t = std::shared_ptr<char>;
    using status_t = status_codes::status_t;

    // Constructors:
    srfc_response(id_t rid, status_t status = status_codes::ok);
    srfc_response(serialized_t builtP, std::size_t reqSize);

    // Setters:
    void setRequestId(id_t rid) noexcept;
    void setStatusCode(status_t code) noexcept;
    void setPayload(payload_t p, std::size_t psize);

    // Getters:
    id_t getRequestId() const noexcept;
    payload_t getPayload(std::size_t* pSize = nullptr) const noexcept;
    status_t getStatusCode() const noexcept;
    std::size_t getHeaderSize() const noexcept;

    // Serialization & deserialization:
    serialized_t serialize(std::size_t* pSize) const;
    void deserialize(serialized_t s, const std::size_t sSize);
    std::string to_string() const;

    //
    void reset();

protected:
    static constexpr const char* protocol_version = "SRFCv1"; 
    static constexpr const char* type = "RES";

    id_t request_id = 0;
    status_t status_code = 0;
    payload_t payload_ptr = nullptr;
    std::size_t payload_size = 0;

}; // class srfc_response 

} // namespace net
#endif