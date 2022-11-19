
#include "includes/srfc_response.hpp"

#include <cstring>
#include <stdexcept>
#include <vector>

#include "includes/utilities/alg.hpp"
#include "includes/utilities/array_deleter.hpp"

namespace net
{

constexpr const char* srfc_response::protocol_version; 
constexpr const char* srfc_response::type;

//
// Constructors:
//

srfc_response::srfc_response(id_t rid, status_t status) :
    request_id(rid),
    status_code(status)
{
}

srfc_response::srfc_response(serialized_t builtP, std::size_t reqSize)
{
    deserialize(builtP, reqSize);
}

//
// Setters:
//

void srfc_response::setRequestId(id_t rid) noexcept
{
    this->request_id = rid;
}

void srfc_response::setStatusCode(status_t code) noexcept
{
    this->status_code = code;
}

void srfc_response::setPayload(payload_t p, std::size_t psize)
{
    this->payload_ptr = p;
    this->payload_size = psize;
}

//
// Getters:
//

srfc_response::id_t 
srfc_response::getRequestId() const noexcept
{
    return request_id;
}

srfc_response::payload_t 
srfc_response::getPayload(std::size_t* pSize) const noexcept
{
    if(pSize != nullptr) {
        *pSize = this->payload_size;
    }

    return this->payload_ptr;
}

srfc_response::status_t 
srfc_response::getStatusCode() const noexcept
{
    return this->status_code;
}

std::size_t srfc_response::getHeaderSize() const noexcept
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
    sz += digits(request_id);
    sz += 1; // add trailing null
    
    sz += std::strlen("PS: ");
    sz += digits(payload_size);
    sz += 1; // add trailing null

    /* add status code size: */
    sz += std::strlen("STATUS: ");
    sz += digits(status_code);
    sz += 1; // add trailing null

    return sz;
}

//
// Serialization & deserialization:
//

srfc_response::serialized_t 
srfc_response::serialize(std::size_t* pSize) const 
{
    const auto head_size = getHeaderSize();
    const auto full_size = head_size + payload_size; 

    // Set pSize value:
    *pSize = full_size;

    // allocate memeory for serialized response:
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
    tmpbuf = std::to_string(request_id);
    copy_and_shift(tmpptr, "RI: ", std::strlen("RI: "));
    copy_and_shift(tmpptr, tmpbuf.c_str(), tmpbuf.size());
    *(tmpptr++) = static_cast<char>(0); // add trailing null

    // Set PS:
    tmpbuf = std::to_string(payload_size);
    copy_and_shift(tmpptr, "PS: ", std::strlen("PS: "));
    copy_and_shift(tmpptr, tmpbuf.c_str(), tmpbuf.size());
    *(tmpptr++) = static_cast<char>(0); // add trailing null

    // Set Status code:
    tmpbuf = std::to_string(status_code);
    copy_and_shift(tmpptr, "STATUS: ", std::strlen("STATUS: "));
    copy_and_shift(tmpptr, tmpbuf.c_str(), tmpbuf.size());
    *(tmpptr++) = static_cast<char>(0); // add trailing null

    // Set payload:
    copy_and_shift(tmpptr, payload_ptr.get(), payload_size);

    return pntr;   
}

void srfc_response::deserialize(serialized_t s, const std::size_t sSize)
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
    dynamic_assert<std::logic_error>(protocolVersion, protocol_version, "Invalid protocol version");

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

    this->request_id = std::stoul(requestIdP.second);

    /*-----------------------------------------------------*/
    /*               Get Payload Size:                     */
    /*-----------------------------------------------------*/
    auto payloadSize = separate_param_val(sstrcpy_and_shift(ptr, rbound));
    dynamic_assert<std::logic_error>(payloadSize.first, "PS", "Invalid header structure");

    this->payload_size = std::stoul(payloadSize.second);

    /*-----------------------------------------------------*/
    /*                Get Status Code:                     */
    /*-----------------------------------------------------*/
    auto stCode = separate_param_val(sstrcpy_and_shift(ptr, rbound));
    dynamic_assert<std::logic_error>(stCode.first, "STATUS", "Invalid header structure");

    this->status_code = std::stoul(stCode.second);

    /*-----------------------------------------------------*/
    /*                  Get Payload:                       */
    /*-----------------------------------------------------*/
    auto payload_pointer = s.get() + sSize - payload_size;

    payload_t tmp(new char[payload_size], array_deleter<char>());
    std::memcpy(tmp.get(), payload_pointer, payload_size);

    this->payload_ptr = std::move(tmp);
}

std::string srfc_response::to_string() const 
{
    std::string res;
    // Add Protocol version
    res += std::string(protocol_version) + "\n"; 
    
    // Add Type:
    res += std::string("TYPE: ") + type + "\n";
    
    // Add RI:
    res += std::string("RI: ") + std::to_string(request_id) + "\n";

    // Add PS:
    res +=  std::string("PS: ") + std::to_string(payload_size) + "\n";

    // Add Status code:
    res +=  std::string("STATUS: ") + std::to_string(status_code) + "\n";

    // Add Payload:
    res += "<Payload> " + std::to_string(payload_size) + " bytes" + "\n";
    return res; 
}

//
//
//

void srfc_response::reset()
{
    request_id = 0;
    status_code = status_codes::none;
    payload_ptr.reset();
    payload_size = 0;
}


} // namespace net
