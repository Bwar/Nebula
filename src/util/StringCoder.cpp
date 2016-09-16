/*******************************************************************************
 * Project:  GongyiLib
 * @file     StringCoder.cpp
 * @brief 
 * @author   bwarliao
 * @date:    2015��3��18��
 * @note
 * Modify history:
 ******************************************************************************/
#include "StringCoder.hpp"

namespace neb
{

unsigned char ToHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else if ((str[i] >= '0' && str[i] <= '9')
                        || (str[i] >= 'A' && str[i] <= 'Z')
                        || (str[i] >= 'a' && str[i] <= 'z'))
        {
            strTemp += str[i];
        }
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        //if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high*16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

std::string CharToHex(char c)
{
    std::string sValue;
    static char MAPX[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    sValue += MAPX[(c >> 4) & 0x0F];
    sValue += MAPX[c & 0x0F];
    return sValue;
}

char HexToChar(const std::string & sHex)
{
    unsigned char c=0;
    for ( unsigned int i=0; i<std::min<unsigned int>(sHex.size(), (unsigned int)2); ++i ) {
        unsigned char c1 = std::toupper(sHex[i]);
        unsigned char c2 = (c1 >= 'A') ? (c1 - ('A' - 10)) : (c1 - '0');
        (c <<= 4) += c2;
    }
    return c;
}

std::string EncodeHexToString(const std::string & sSrc)
{
    std::string result;
    std::string strSrc = std::string("+")+sSrc;

    std::string::const_iterator pos;
    for(pos = strSrc.begin(); pos != strSrc.end(); ++pos)
    {
        result.append(CharToHex(*pos));
    }
    return result;
}

std::string DecodeStringToHex(const std::string & sSrc)
{
    std::string result;
    std::string::const_iterator pos;
    pos = sSrc.begin();
    for(pos = pos + 2; pos != sSrc.end(); )
    {
        result.append(1, HexToChar(std::string(pos,pos+2)));
        pos = pos+2;
    }

    return result;
}

void EncodeParameter(const std::map<std::string, std::string>& mapParameters, std::string& strParameter)
{
    strParameter.clear();
    for (std::map<std::string, std::string>::const_iterator iter = mapParameters.begin();
                    iter != mapParameters.end(); ++iter)
    {
        if (strParameter.size() > 0)
        {
            strParameter += "&";
        }
        strParameter += iter->first;
        strParameter += "=";
        strParameter += iter->second;
    }
}

void DecodeParameter(const std::string& strParameter, std::map<std::string, std::string>& mapParameters)
{
    mapParameters.clear();
    std::string strKey;
    std::string strValue;
    bool bIsValue = false;
    for (size_t i = 0; i < strParameter.size(); ++i)
    {
        if ('&' == strParameter[i])
        {
            if (strKey.size() > 0)
            {
                mapParameters.insert(std::pair<std::string, std::string>(strKey, strValue));
            }
            strKey.clear();
            strValue.clear();
            bIsValue = false;
        }
        else if ('=' == strParameter[i])
        {
            bIsValue = true;
        }
        else
        {
            if (bIsValue)
            {
                strValue += strParameter[i];
            }
            else
            {
                strKey += strParameter[i];
            }
        }
    }
    if (strKey.size() > 0)
    {
        mapParameters.insert(std::pair<std::string, std::string>(strKey, strValue));
    }
}

} /* namespace neb */
