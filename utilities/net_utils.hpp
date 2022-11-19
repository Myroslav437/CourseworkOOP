#ifndef NET_UTILS_HPP
#define NET_UTILS_HPP

#include <stdexcept>
#include <algorithm>

#include "../network/includes/srfc_request.hpp"
#include "../network/includes/srfc_response.hpp"

#include "alg.hpp"
#include "array_deleter.hpp"

namespace net {

using payload_t = srfc_connection::payload_t;

inline bool is_valid_message(const srfc_request::serialized_t& message, std::size_t mSize) noexcept
{
    try{
        // raw pointer to copy data. Should NOT be deleted:
        auto* ptr = message.get();  
        const auto* const rbound = ptr +  mSize;

        /*-----------------------------------------------------*/
        /*               Check Preamble:                       */
        /*-----------------------------------------------------*/
        if(mSize < 32) {
            return false;
        }

        // Throws std::invalid_argument if no conversion could be performed
        const std::size_t preamble_value = std::stoul(std::string(ptr, ptr + 32)); 
        if(preamble_value !=  mSize) {
            // Serialized size and preamble value differs
            return false;
        }
        ptr += 32;

        /*-----------------------------------------------------*/
        /*           Check Protocol version:                   */
        /*-----------------------------------------------------*/
        // throws std::out_of_range if invalid string
        const auto protocolVersion = sstrcpy_and_shift(ptr, rbound);
        if(protocolVersion != "SRFCv1") {
            // Protocols don't match
            return false;
        }

        /*-----------------------------------------------------*/
        /*                  Check Type:                        */
        /*-----------------------------------------------------*/
        // throws std::invalid_argument, std::out_of_range if invalid string
        const auto typeP = separate_param_val(sstrcpy_and_shift(ptr, rbound));
        if(typeP.first != "TYPE") {
            // Invalid header structure
            return false;
        }
        if(typeP.second != "REQ" && typeP.second != "RES") {
            // Invalid type
            return false;    
        }
        
        /*-----------------------------------------------------*/
        /*                Check Request ID:                    */
        /*-----------------------------------------------------*/
        // throws std::invalid_argument, std::out_of_range if invalid string
        const auto requestIdP = separate_param_val(sstrcpy_and_shift(ptr, rbound));
        if(requestIdP.first != "RI") {
            // Invalid header structure
            return false;    
        }
        // Throws std::invalid_argument if no conversion could be performed
        std::stoul(requestIdP.second);

        /*-----------------------------------------------------*/
        /*              Check Payload Size:                    */
        /*-----------------------------------------------------*/
        // throws std::invalid_argument, std::out_of_range if invalid string
        const auto payloadSize = separate_param_val(sstrcpy_and_shift(ptr, rbound));
        if(payloadSize.first != "PS") {
            // Invalid header structure
            return false;    
        }
        // Throws std::invalid_argument if no conversion could be performed
        const auto payload_size = std::stoul(payloadSize.second);

        /*-----------------------------------------------------*/
        /*                 Check Method: (for requests)        */
        /*-----------------------------------------------------*/
        if(typeP.second == "REQ") {
            // throws std std::out_of_range if invalid string
            sstrcpy_and_shift(ptr, rbound);
        }

        /*-----------------------------------------------------*/
        /*          Check Parameters: (for requests)           */
        /*-----------------------------------------------------*/
        if(typeP.second == "REQ") {
            while (ptr < rbound - payload_size) {
                // throws std::invalid_argument, std::out_of_range if invalid string
                separate_param_val(sstrcpy_and_shift(ptr, rbound));
            }
        }

        /*-----------------------------------------------------*/
        /*      Check Status Code: (for responses)             */
        /*-----------------------------------------------------*/
        if(typeP.second == "RES") {
            // throws std::invalid_argument, std::out_of_range if invalid string
            const auto stCode = separate_param_val(sstrcpy_and_shift(ptr, rbound));
            if(stCode.first != "STATUS") {
                // Invalid header structure
                return false;   
            }
            // Throws std::invalid_argument if no conversion could be performed
            std::stoul(stCode.second);
        }
    }
    catch(...) {
        return false;
    }
    return true;
}

// d should point to a block of memory of size at least <preamble size> (32)
// Throws std::invalid_argument if no conversion could be performed
inline std::size_t get_size_from_preamble(const char* d) {
    std::size_t message_size = std::stoul(std::string(d, d + 32));
    return message_size;
}

inline std::string extract_type(srfc_request::serialized_t message, std::size_t size)
{
    // raw pointer to read data. Should NOT be deleted
    auto ptr = message.get();
    auto rbound = ptr + size;

    // omit Preamble:
    dynamic_assert<std::logic_error>(
        [&size]{return size >= 32;}, "Message size is less than the preamble size (32)");

    ptr += 32; 

    // omit Protocol version:
    auto protocolVersion = ::sstrcpy_and_shift(ptr, rbound); // throws if out of bound

    // Get Type:
    auto type = separate_param_val(sstrcpy_and_shift(ptr, nullptr)); // throws if out of bound
    dynamic_assert<std::logic_error>(type.first, "TYPE", "Invalid message structure");

    return type.second;
}

inline std::string get_param(const srfc_connection::params_t& par, const std::string& parName) {
    auto val = std::find_if(par.cbegin(), par.cend(), 
        [&par, &parName](const auto& p){return p.first == parName;});

    if(val == par.end()) {
        throw std::out_of_range("Param " + parName + " not found.");                
    }
    
    return val->second;
}

inline void set_payload_msg(payload_t* pl, std::string message, std::size_t* plsize)
{
    *pl = payload_t(new char[message.size()], array_deleter<char>());
    std::memcpy(pl->get(), message.c_str(), message.length());
    *plsize = message.length();
}

inline void set_payload_data(payload_t* pl, std::size_t* plsize, const char* data, std::size_t size)
{
    *pl = payload_t(new char[size], array_deleter<char>());
    std::memcpy(pl->get(), data, size);
    *plsize = size;
}

} // namespace net 

#endif