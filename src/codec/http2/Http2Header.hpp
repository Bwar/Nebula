/*******************************************************************************
 * Project:  Nebula
 * @file     Http2Header.hpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-03
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_HTTP2_HTTP2HEADER_HPP_
#define SRC_CODEC_HTTP2_HTTP2HEADER_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include "util/CBuffer.hpp"

namespace neb
{

enum E_H2_HPACK_CONDITION
{
    H2_HPACK_CONDITION_INDEXED_HEADER                   = 0x80,     ///< rfc7541 6.1
    H2_HPACK_CONDITION_LITERAL_HEADER_WITH_INDEXING     = 0x40,     ///< rfc7541 6.2.1
    H2_HPACK_CONDITION_LITERAL_HEADER_WITHOUT_INDEXING  = 0x00,     ///< rfc7541 6.2.2
    H2_HPACK_CONDITION_LITERAL_HEADER_NEVER_INDEXED     = 0x10,     ///< rfc7541 6.2.3
    H2_HPACK_CONDITION_DYNAMIC_TABLE_SIZE_UPDATE        = 0x20,     ///< rfc7541 6.3
};

enum E_H2_HPACK_PREFIX
{
    H2_HPACK_PREFIX_0_BITS = 0x00,
    H2_HPACK_PREFIX_1_BITS = 0x01,
    H2_HPACK_PREFIX_2_BITS = 0x03,
    H2_HPACK_PREFIX_3_BITS = 0x07,
    H2_HPACK_PREFIX_4_BITS = 0x0F,
    H2_HPACK_PREFIX_5_BITS = 0x1F,
    H2_HPACK_PREFIX_6_BITS = 0x3F,
    H2_HPACK_PREFIX_7_BITS = 0x7F,
    H2_HPACK_PREFIX_8_BITS = 0xFF,
};

enum E_H2_HEADER_TYPE
{
    H2_HEADER_PSEUDO        = 0x01,
    H2_HEADER_NORMAL        = 0x02,
    H2_HEADER_TRAILER       = 0x04,
};

class Http2Header
{
public:
    Http2Header(const std::string& strName, const std::string& strValue);
    Http2Header(const Http2Header& rHeader) = delete;
    Http2Header(Http2Header&& rHeader);
    virtual ~Http2Header();

    Http2Header& operator=(const Http2Header& rHeader) = delete;
    Http2Header& operator=(Http2Header&& rHeader);

    inline const std::string& Name() const
    {
        return(m_strName);
    }

    inline const std::string& Value() const
    {
        return(m_strValue);
    }

    inline size_t HpackSize() const
    {
        return(m_uiHpackSize);
    }

private:
    std::string m_strName;
    std::string m_strValue;
    size_t m_uiHpackSize;

public:
    /**
     * @brief Pseudocode to represent an integer I
     * @param[in] uiValue
     * @param[in] uiPrefix 2^N-1, The number of bits of the prefix (called N) is a parameter of the integer representation.
     * @param[in] cBits bits of the prefix byte.
     * @param[in,out] pBuff encode buffer.
     */
    static void EncodeInt(size_t uiValue, size_t uiPrefix, char cBits, CBuffer* pBuff);

    /**
     * @brief Pseudocode to decode an integer I
     * @param[in] iPrefixMask N bits of 1 and 8-N bits of zero
     * @param[in,out] pBuff decode buffer
     */
    static int DecodeInt(int iPrefixMask, CBuffer* pBuff);

    /**
     * @brief encode string literal
     * @param[in] strLiteral header name or header value string literal
     * @param[in,out] pBuff pack string
     */
    static void EncodeStringLiteral(const std::string& strLiteral, CBuffer* pBuff);

    /**
     * @brief encode string literal
     * @param[in] strLiteral header name or header value string literal
     * @param[in,out] pBuff pack string
     */
    static void EncodeStringLiteralWithHuffman(const std::string& strLiteral, CBuffer* pBuff);

    /**
     * @brief decode string literal
     * @param[in,out] pBuff pack string
     * @param[out] strLiteral header name or header value string literal
     * @param[out] bWithHuffman user huffman or not
     */
    static bool DecodeStringLiteral(CBuffer* pBuff, std::string& strLiteral, bool& bWithHuffman);

    static size_t GetStaticTableIndex(const std::string& strHeaderName, const std::string& strHeaderValue = "");

public:
    static const size_t sc_uiMaxStaticTableIndex;
    static const std::vector<std::pair<std::string, std::string>> sc_vecStaticTable;
    static const std::unordered_map<std::string, size_t> sc_mapStaticTable;
};

} /* namespace neb */

#endif /* SRC_CODEC_HTTP2_HTTP2HEADER_HPP_ */

