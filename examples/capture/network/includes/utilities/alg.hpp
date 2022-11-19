#ifndef ALG_HPP
#define ALG_HPP

#include <cstring>
#include <string>
#include <type_traits>


 // copies sz bytes from s to t and shifts t:
inline void copy_and_shift(char*& t, const char* s, std::size_t sz)
{
    if(t != nullptr && s != nullptr && sz != 0) {
        std::memcpy(t, s, sz);
        t += sz;
    }

    return;
};

// returns std::string of chars from p to the first null and
// moves p to the beginning of the next substring
// throws std::out_of_range if out of rbound
inline std::string sstrcpy_and_shift(char*& p, const char* const rbound = nullptr)
{
    auto* startp = p;

    // move p to the next null:
    while(*p != static_cast<char>(0)) {
        if(rbound != nullptr && p >= rbound) {
            throw std::out_of_range("Invalid serialized request: out of bounds error");
        }
        ++p;
    };
    std::string res(startp, p);
    ++p; // set to the begining of the next substring

    return res;
};

// retrun separated paramName and paramVal from a string
// throws std::invalid_argument if string is ill-formed
inline std::pair<std::string, std::string> separate_param_val(const std::string& str, const char* sep = ": ")
{
    auto pos = str.find_first_of(sep);
    const std::size_t sepSize = std::strlen(sep);

    if(pos != std::string::npos && pos + sepSize < str.length() && pos > 0) {
        auto pName = str.substr(0, pos);
        auto pVal = str.substr(pos + sepSize, str.length());

        return std::make_pair(pName, pVal);
    }
    else {
        throw std::invalid_argument(std::string("Ill-formed line. No ") + sep + " character was found");
    }
};

// Returns amount of decimal digits in a number
// Assuming that 0 has 1 digits
// Number should be an integral type
template <
    typename IntT,
    typename = typename std::enable_if<std::is_integral<IntT>::value, IntT>::type
    >
inline std::size_t digits(IntT num)
{
    std::size_t digits = 0; 
    do { 
        num /= 10; 
        ++digits; 
    } while (num != 0);

    return digits;
};

// Throws exception of type ExcT and what() = excMessage if val1 != val2
template <typename ExcT, typename T1, typename T2>
inline void dynamic_assert(const T1& val1, const T2& val2, std::string excMessage)
{
    if(val1 != val2) {
        throw ExcT{excMessage};
    }
}

// Throws exception of type ExcT and what() = excMessage if val1 != val2
template <typename ExcT, typename Pred> // is_invocable, invoke - from C++17. C++14 is being used :(
inline void dynamic_assert(Pred p, std::string excMessage)
{
    if(!p()) {
        throw ExcT{excMessage};
    }
}

#endif