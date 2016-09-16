/*******************************************************************************
* Project:  Nebula
* @file     CodecUtil.hpp
* @brief 
* @author   Bwar
* @date:    2016年9月2日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_CODEC_CODECUTIL_HPP_
#define SRC_CODEC_CODECUTIL_HPP_

#include <netinet/in.h>
#include <endian.h>

namespace neb
{

unsigned long long ntohll(unsigned long long val);
unsigned long long htonll(unsigned long long val);

}

#endif /* SRC_CODEC_CODECUTIL_HPP_ */
