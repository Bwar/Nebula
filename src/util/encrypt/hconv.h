/*******************************************************************************
* Project:  nebula
* @file     hconv.h
* @brief 
* @author   lbh
* @date:    2015年12月15日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_UTIL_ENCRYPT_HCONV_H_
#define SRC_UTIL_ENCRYPT_HCONV_H_

#include <stdio.h>

/**
 * @brief Convert a buffer from ascii hex to bytes.
 * @param from       ascii hex string
 * @param to         bytes string
 * @param pTo_length Set pTo_length to the byte length of the result.
 * @return eturn 1 if everything went OK.
 */
int hex_to_bytes(char* from, char* to, int* pTo_length);


/**
 * @brief Convert a buffer from bytes to ascii hex.
 * @param from          bytes string
 * @param from_length   bytes string length
 * @param to            ascii hex string
 * @return Return 1 if everything went OK.
 */
int bytes_to_hex(char* from, int from_length, char* to);


#endif /* SRC_UTIL_ENCRYPT_HCONV_H_ */
