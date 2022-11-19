#ifndef SRFC_REQUEST_HPP
#define SRFC_REQUEST_HPP

#include <string>
#include <vector>
#include <utility>
#include <atomic>
#include <memory>

namespace net
{

class srfc_request 
{
public:
    using params_t = std::vector<std::pair<std::string, std::string>>;
    using id_t = unsigned long;
    using payload_t = std::shared_ptr<char>;
    using serialized_t = std::shared_ptr<char>;

    // Copy constructors:
    srfc_request(const srfc_request& other);
    srfc_request& operator=(const srfc_request& other);

    // Default constructor & parameterized constructors:
    srfc_request();
    srfc_request(const std::string& methodName);
    srfc_request(serialized_t builtP, std::size_t reqSize);
    
    // Move constructor & move assignment operator:
    srfc_request(srfc_request&& other) noexcept;
    srfc_request& operator=(srfc_request&& other) noexcept;

    // Setters:
    bool setMethod(const std::string& methodName);
    bool setParams(const params_t& params);
    void setPayload(payload_t p, std::size_t psize);

    // Getters:
    const std::string& getMethod() const noexcept;
    const params_t& getParams() const noexcept;
    id_t getRequestId() const noexcept;
    payload_t getPayload(std::size_t* pSize = nullptr) const noexcept;
    std::size_t getHeaderSize() const noexcept;

    // Serialization & deserialization:
    serialized_t serialize(std::size_t* pSize) const;
    void deserialize(serialized_t s, const std::size_t sSize);
    std::string to_string() const;

    // 
    bool addParam(const std::string& param, const std::string& paramVal);
    bool removeParam(const std::string& paramName);
    void reset();

private:
    bool validMethod(const std::string& methodName);
    bool validParams(const params_t& params);

protected:
    static constexpr const char* protocol_version = "SRFCv1"; 
    static constexpr const char* type = "REQ";

    static std::atomic<id_t> req_id_counter;
    id_t my_request_id;

    std::string method_name;
    params_t parameters;
    payload_t payload_ptr = nullptr;
    std::size_t payload_size = 0;
}; // class srfc_request 

} // namespace net
#endif