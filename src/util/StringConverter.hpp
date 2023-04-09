/*******************************************************************************
 * Project:  Nebula
 * @file     StringConverter.hpp
 * @brief
 * @author   nebim
 * @date:    2020-05-02
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_UTIL_STRINGCONVERTER_HPP_
#define SRC_UTIL_STRINGCONVERTER_HPP_

#include "Definition.hpp"

#define _TO_NUM(v) ((v) ^ '0')
#define _IS_NUM(v) (v >= '0' && v <= '9')

namespace neb
{

static const char gc_cDigitForConvertor[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

class StringConverter
{
public:
    StringConverter();
    virtual ~StringConverter();

    template<typename TInt>
    static inline TInt RapidAtoi(const char* str)
    {
        int sign = 1;
        TInt res = 0;
        int k = 0;
        if (str[0] == '-')
        {
            sign = -1;
            k = 1;
        }
        for (; ; k += 4)
        {
            if (not _IS_NUM(str[k]))
            {
                return(res * sign);
            }
            else if (not _IS_NUM(str[k + 1]))
            {
                return((res * 10 + _TO_NUM(str[k])) * sign);
            }
            else if (not _IS_NUM(str[k + 2]))
            {
                return((res * 100 + _TO_NUM(str[k]) * 10 + _TO_NUM(str[k + 1])) * sign);
            }
            else if (not _IS_NUM(str[k + 3]))
            {
                return((res * 1000 + _TO_NUM(str[k]) * 100
                        + _TO_NUM(str[k + 1]) * 10 + _TO_NUM(str[k + 2])) * sign);
            }
            else
            {
                res = res * 10000 + _TO_NUM(str[k]) * 1000 + _TO_NUM(str[k + 1])
                        * 100 + _TO_NUM(str[k + 2]) * 10 + _TO_NUM(str[k + 3]);
            }
        }
        return(res * sign);
    }

    template<typename TInt>
    static inline TInt RapidAtoi(const char* str, int iDefaultValue)
    {
        int sign = 1;
        TInt res = 0;
        int k = 0;
        if (str[0] == '-')
        {
            sign = -1;
            k = 1;
        }
        for (; ; k += 4)
        {
            if (not _IS_NUM(str[k]))
            {
                if (str[k] == '\0')
                {
                    return(res * sign);
                }
                return(iDefaultValue);
            }
            else if (not _IS_NUM(str[k + 1]))
            {
                if (str[k + 1] == '\0')
                {
                    return((res * 10 + _TO_NUM(str[k])) * sign);
                }
                return(iDefaultValue);
            }
            else if (not _IS_NUM(str[k + 2]))
            {
                if (str[k + 2] == '\0')
                {
                    return((res * 100 + _TO_NUM(str[k]) * 10 + _TO_NUM(str[k + 1])) * sign);
                }
                return(iDefaultValue);
            }
            else if (not _IS_NUM(str[k + 3]))
            {
                if (str[k + 3] == '\0')
                {
                    return((res * 1000 + _TO_NUM(str[k]) * 100
                            + _TO_NUM(str[k + 1]) * 10 + _TO_NUM(str[k + 2])) * sign);
                }
                return(iDefaultValue);
            }
            else
            {
                res = res * 10000 + _TO_NUM(str[k]) * 1000 + _TO_NUM(str[k + 1])
                        * 100 + _TO_NUM(str[k + 2]) * 10 + _TO_NUM(str[k + 3]);
            }
        }
        return(res * sign);
    }

    static uint32 Digits10(uint64 v) {
        if (v < 10) return 1;
        if (v < 100) return 2;
        if (v < 1000) return 3;
        if (v < 1000000000000) {    // 10^12
            if (v < 100000000) {    // 10^7
                if (v < 1000000) {  // 10^6
                    if (v < 10000) return 4;
                    return 5 + (v >= 100000); // 10^5
                }
                return 7 + (v >= 10000000); // 10^7
            }
            if (v < 10000000000) {  // 10^10
                return 9 + (v >= 1000000000); // 10^9
            }
            return 11 + (v >= 100000000000); // 10^11
        }
        return 12 + Digits10(v / 1000000000000); // 10^12
    }

    static uint32 uint64ToAscii(uint64 value, char* dst) {

        const uint32 length = Digits10(value);
        uint32 next = length - 1;

        while (value >= 100) {
            const uint32 i = (value % 100) * 2;
            value /= 100;
            dst[next - 1] = gc_cDigitForConvertor[i];
            dst[next] = gc_cDigitForConvertor[i + 1];
            next -= 2;
        }
        // Handle last 1-2 digits
        if (value < 10) {
            dst[next] = '0' + uint32(value);
        } else {
            uint32 i = uint32(value) * 2;
            dst[next - 1] = gc_cDigitForConvertor[i];
            dst[next] = gc_cDigitForConvertor[i + 1];
        }
        return length;
    }

    static void uint64ToAscii(uint64_t value, uint32 value_len, char* dst)
    {
        uint32 next = value_len - 1;

        while (value >= 100) {
            const uint32 i = (value % 100) * 2;
            value /= 100;
            dst[next - 1] = gc_cDigitForConvertor[i];
            dst[next] = gc_cDigitForConvertor[i + 1];
            next -= 2;
        }
        // Handle last 1-2 digits
        if (value < 10) {
            dst[next] = '0' + uint32(value);
        } else {
            uint32 i = uint32(value) * 2;
            dst[next - 1] = gc_cDigitForConvertor[i];
            dst[next] = gc_cDigitForConvertor[i + 1];
        }
    }
};


} /* namespace neb */

#endif /* SRC_UTIL_STRINGCONVERTER_HPP_ */

