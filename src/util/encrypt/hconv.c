/*******************************************************************************
* Project:  nebula
* @file     hconv.c
* @brief 
* @author   lbh
* @date:    2015年12月15日
* @note
* Modify history:
******************************************************************************/

#include "hconv.h"

int hex_to_bytes(char* from, char* to, int* pTo_length)
{
    char *pHex; /* Ptr to next hex character. */
    char *pByte; /* Ptr to next resulting byte. */
    int byte_length = 0;
    int value;

    pByte = to;
    for (pHex = from; *pHex != 0; pHex += 2)
    {
        if (1 != sscanf(pHex, "%02x", &value))
            return (0);
        *pByte++ = ((char) (value & 0xFF));
        byte_length++;
    }
    *pTo_length = byte_length;
    return (1);
}


int bytes_to_hex(char* from, int from_length, char* to)
{
    char *pHex; /* Ptr to next hex character. */
    char *pByte; /* Ptr to next resulting byte. */
    int value;

    pHex = to;
    for (pByte = from; from_length > 0; from_length--)
    {
        value = *pByte++ & 0xFF;
        (void) sprintf(pHex, "%02x", value);
        pHex += 2;
    }
    return (1);
}
