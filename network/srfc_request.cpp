#include "includes/srfc_request.hpp"

#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <vector>

#include "../utilities/array_deleter.hpp"
#include "../utilities/alg.hpp"

namespace net 
{

//
// Private static members initialization:
//

std::atomic<srfc_request::id_t> srfc_request::req_id_counter {1};
constexpr const char* srfc_request::protocol_version; 
constexpr const char* srfc_request::type;

//
// Constructors:
//

srfc_request::srfc_request() :
    my_request_id(req_id_counter++)  
{
}

srfc_request::srfc_request(const std::string& methodName) :
    my_request_id(req_id_counter++)
{
    if(validMethod(methodName)) {
        this->method_name = methodName;
    }
    else{
        throw std::invalid_argument("Invalild method name");
    }
}

srfc_request::srfc_request(serialized_t builtP, std::size_t reqSize)
{
    deserialize(builtP, reqSize);
}

srfc_request::srfc_request(srfc_request&& other) noexcept
{
    *this = std::move(other);
}

srfc_request::srfc_request(const srfc_request& other)
{
    *this = other;
}

srfc_request& srfc_request::operator=(const srfc_request& other)
{
    my_request_id = other.my_request_id;
    method_name = other.method_name;
    
    parameters = other.parameters;
    payload_ptr = other.payload_ptr;

    payload_size = other.payload_size;

    return *this;
}

//
// Operator overloads:
//

srfc_request& srfc_request::operator=(srfc_request&& other) noexcept
{
    my_request_id = other.my_request_id;
    other.my_request_id = 0;

    method_name = std::move(other.method_name);
    other.method_name = "";

    parameters = std::move(other.parameters);
    payload_ptr = std::move(other.payload_ptr);

    payload_size = other.payload_size;
    other.payload_size = 0;

    return *this;
}

//
// Setters:
//

bool srfc_request::setMethod(const std::string& methodName)
{
    if(validMethod(methodName)) {
        this->method_name = methodName;
        return true;
    }
    return false;
}

bool srfc_request::setParams(const params_t& params)
{
    if(validParams(params)) {
        this->parameters = params;
        return true;
    }
    return false;
}

void srfc_request::setPayload(payload_t p, std::size_t psize)
{
    this->payload_ptr = p;
    this->payload_size = psize;
}

//
// Getters:
//

const std::string& srfc_request::getMethod() const noexcept
{
    return this->method_name;
}

const srfc_request::params_t& 
srfc_request::getParams() const noexcept
{
    return this->parameters;
}

srfc_request::id_t 
srfc_request::getRequestId() const noexcept
{
    return this->my_request_id;
}

srfc_request::payload_t 
srfc_request::getPayload(std::size_t* pSize) const noexcept
{
    if(pSize != nullptr) {
        *pSize = this->payload_size;
    }

    return this->payload_ptr;
}

std::size_t srfc_request::getHeaderSize() const noexcept
{
    std::size_t sz = 0;

    /* add Preamble size*/
    sz += 32;

    /* add protocol version size: */
    sz += std::strlen(protocol_version);
    sz += 1; // add trailing null

    /* add type size: */
    sz += std::strlen("TYPE: ");
    sz += std::strlen(type);
    sz += 1; // add trailing null

    /* add RI, PS sizes: */
    sz += std::strlen("RI: ");
    sz += digits(my_request_id);
    sz += 1; // add trailing null

    sz += std::strlen("PS: ");
    sz += digits(payload_size);
    sz += 1; // add trailing null

    /* add method size: */
    sz += method_name.size();
    sz += 1; // add trailing null

    /* add params sizes: */
    for(const auto& p : parameters) {
        sz += p.first.size();
        sz += std::strlen(": ");
        sz += p.second.size();
        sz += 1; // add trailing null
    }
    
    return sz;
}

//
// Serialization & deserialization:
//

srfc_request::serialized_t srfc_request::serialize(std::size_t* pSize) const
{
    const auto head_size = getHeaderSize();
    const auto full_size = head_size + payload_size;

    // Set pSize value:
    *pSize = full_size;

    // allocate memeory for serialized request:
    serialized_t pntr(new char[full_size], array_deleter<char>());
    auto tmpptr = pntr.get();   // raw pointer to write data. Should NOT be deleted.

    // buffer for string for storing serialized integers:
    std::string tmpbuf;

    // Set preamble:
    tmpbuf = std::string(32 - digits(full_size), '0') +  std::to_string(full_size);
    copy_and_shift(tmpptr, tmpbuf.c_str(), 32);

    // Set protocol version:
    copy_and_shift(tmpptr, protocol_version, std::strlen(protocol_version));
    *(tmpptr++) = static_cast<char>(0); // add trailing null

    // Set type:
    copy_and_shift(tmpptr, "TYPE: ", std::strlen("TYPE: "));
    copy_and_shift(tmpptr, type, std::strlen(type));
    *(tmpptr++) = static_cast<char>(0); // add trailing null

    // Set RI:
    tmpbuf = std::to_string(my_request_id);
    copy_and_shift(tmpptr, "RI: ", std::strlen("RI: "));
    copy_and_shift(tmpptr, tmpbuf.c_str(), tmpbuf.size());
    *(tmpptr++) = static_cast<char>(0); // add trailing null

    // Set PS:
    tmpbuf = std::to_string(payload_size);
    copy_and_shift(tmpptr, "PS: ", std::strlen("PS: "));
    copy_and_shift(tmpptr, tmpbuf.c_str(), tmpbuf.size());
    *(tmpptr++) = static_cast<char>(0); // add trailing null

    // Set Method:
    copy_and_shift(tmpptr, method_name.c_str(), method_name.size());
    *(tmpptr++) = static_cast<char>(0); // add trailing null

    // Set parameters:
    for(const auto& p : parameters) {
        copy_and_shift(tmpptr, p.first.c_str(), p.first.size());
        copy_and_shift(tmpptr, ": ", std::strlen(": "));
        copy_and_shift(tmpptr, p.second.c_str(), p.second.size());
        *(tmpptr++) = static_cast<char>(0); // add trailing null
    }

    // Set payload:
    copy_and_shift(tmpptr, payload_ptr.get(), payload_size);

    return pntr;
}

void srfc_request::deserialize(serialized_t s, const std::size_t sSize)
{
    const auto* const rbound = s.get() +  sSize;

    // raw pointer to copy data. Should NOT be deleted:
    auto* ptr = s.get();  

    /*-----------------------------------------------------*/
    /*                 Get Preamble:                       */
    /*-----------------------------------------------------*/
    dynamic_assert<std::out_of_range>(
        [&sSize]{ return sSize >= 32;}, "Serialized response size can't be less than 32");

    std::vector<char> preamble(ptr, ptr + 32);
    std::size_t preamble_value = std::stoul(std::string(preamble.begin(), preamble.end()));

    dynamic_assert<std::logic_error>(preamble_value, sSize, "Serialized size and preamble value differs");

    ptr += 32;

    /*-----------------------------------------------------*/
    /*            Get Protocol version:                    */
    /*-----------------------------------------------------*/
    auto protocolVersion = sstrcpy_and_shift(ptr, rbound);
    dynamic_assert<std::logic_error>(protocolVersion, this->protocol_version, "Invalid protocol version");

    /*-----------------------------------------------------*/
    /*                    Get Type:                        */
    /*-----------------------------------------------------*/
    auto typeP = separate_param_val(sstrcpy_and_shift(ptr, rbound));
    dynamic_assert<std::logic_error>(typeP.first, "TYPE", "Invalid header structure");
    dynamic_assert<std::logic_error>(typeP.second, this->type, "Invalid type value");
    
    /*-----------------------------------------------------*/
    /*                  Get Request ID:                     */
    /*-----------------------------------------------------*/
    auto requestIdP = separate_param_val(sstrcpy_and_shift(ptr, rbound));
    dynamic_assert<std::logic_error>(requestIdP.first, "RI", "Invalid header structure");

    this->my_request_id = std::stoul(requestIdP.second);

    /*-----------------------------------------------------*/
    /*               Get Payload Size:                     */
    /*-----------------------------------------------------*/
    auto payloadSize = separate_param_val(sstrcpy_and_shift(ptr, rbound));
    dynamic_assert<std::logic_error>(payloadSize.first, "PS", "Invalid header structure");

    this->payload_size = std::stoul(payloadSize.second);

    /*-----------------------------------------------------*/
    /*                  Get Method:                        */
    /*-----------------------------------------------------*/
    this->method_name = sstrcpy_and_shift(ptr, rbound);

    /*-----------------------------------------------------*/
    /*                  Get Parameters:                    */
    /*-----------------------------------------------------*/
    while (ptr < rbound - payload_size) {
        auto param = separate_param_val(sstrcpy_and_shift(ptr, rbound));
        this->parameters.emplace_back(std::move(param));
    }

    /*-----------------------------------------------------*/
    /*                  Get Payload:                    */
    /*-----------------------------------------------------*/
    auto payload_pointer = s.get() + sSize - payload_size;

    payload_t tmp(new char[payload_size], array_deleter<char>());
    std::memcpy(tmp.get(), payload_pointer, payload_size);

    this->payload_ptr = std::move(tmp);
}

std::string srfc_request::to_string() const 
{
    std::string res;
    // Add Protocol version
    res += std::string(protocol_version) + "\n"; 
    
    // Add Type:
    res += std::string("TYPE: ") + type + "\n";
    
    // Add RI:
    res += std::string("RI: ") + std::to_string(my_request_id) + "\n";

    // Add PS:
    res +=  std::string("PS: ") + std::to_string(payload_size) + "\n";

    // Add Method:
    res += method_name + "\n";

    // Add Parameters:
    for(const auto& p : parameters) {
        res += std::string(p.first.begin(), p.first.end());
        res += std::string(": ");
        res += std::string(p.second.begin(), p.second.end()) + "\n";
    }

    // Add Payload:
    res += "<Payload> " + std::to_string(payload_size) + " bytes" + "\n";
    return res; 
}

//
//
//

bool srfc_request::addParam(const std::string& param, const std::string& paramVal)
{
    parameters.push_back(std::make_pair(param, paramVal));
    if(validParams(parameters)) {
        return true;
    }
    else {
        // return to previous state
        parameters.pop_back();
        return false;
    }
}

bool srfc_request::removeParam(const std::string& paramName)
{
    auto it = std::find_if(parameters.begin(), parameters.end(), [&paramName](const auto& p){return p.first == paramName;});
    if(it != parameters.end()) {
        // no need to validate as a valid parameter list after erasion of an element is valid 
        parameters.erase(it);
        return true;
    }
    else{
        return false;
    }

}

void srfc_request::reset()
{
    // not changing my_request_id as the object can be reused
    method_name = "";
    parameters.clear();
    payload_ptr.reset();
    payload_size = 0;
}

// Not yet implemeted
bool srfc_request::validMethod(const std::string& methodName) 
{
    return true;
}

// Not yet implemeted
bool srfc_request::validParams(const params_t& params) 
{
    return true;
}

} // namespace net 