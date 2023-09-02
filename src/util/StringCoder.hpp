/*******************************************************************************
 * Project:  GongyiLib
 * @file     StringCoder.hpp
 * @brief 
 * @author   bwarliao
 * @date:    2015��3��18��
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef STRINGCODER_HPP_
#define STRINGCODER_HPP_

#include <assert.h>
#include <string>
#include <map>
#include <vector>

namespace neb
{

unsigned char ToHex(unsigned char x);

unsigned char FromHex(unsigned char x);

std::string UrlEncode(const std::string& str);

std::string UrlDecode(const std::string& str);

std::string CharToHex(char c);

char HexToChar(const std::string & sHex);

std::string EncodeHexToString(const std::string & sSrc);

std::string DecodeStringToHex(const std::string & sSrc);

void EncodeParameter(const std::map<std::string, std::string>& mapParameters, std::string& strParameter);

void DecodeParameter(const std::string& strParameter, std::map<std::string, std::string>& mapParameters);

void Split(const std::string& strSrc, const std::string& strPattern, std::vector<std::string>& vecDest);

std::string& Trim(std::string &s);

} /* namespace neb */

#endif /* STRINGCODER_HPP_ */
