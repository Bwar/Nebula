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

#define _TO_NUM(v) ((v) ^ '0')
#define _IS_NUM(v) (v >= '0' && v <= '9')

namespace neb
{

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
};

} /* namespace neb */

#endif /* SRC_UTIL_STRINGCONVERTER_HPP_ */

