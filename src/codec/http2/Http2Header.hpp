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
    size_t m_uiHpackSize;
    std::string m_strName;
    std::string m_strValue;

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
    static const size_t sc_uiMaxStaticTableIndex = 61;
    static const std::vector<
        std::pair<std::string, std::string>> sc_vecStaticTable = {
                {"unknow", "undefine"},
                {":authority", ""},                         ///< 1
                {":method", "GET"},                         ///< 2
                {":method", "POST"},                        ///< 3
                {":path", "/"},                             ///< 4
                {":path", "/index.html"},                   ///< 5
                {":scheme", "http"},                        ///< 6
                {":scheme", "https"},                       ///< 7
                {":status", "200"},                         ///< 8
                {":status", "204"},                         ///< 9
                {":status", "206"},                         ///< 10
                {":status", "304"},                         ///< 11
                {":status", "400"},                         ///< 12
                {":status", "404"},                         ///< 13
                {":status", "500"},                         ///< 14
                {"accept-charset", ""},                     ///< 15
                {"accept-encoding", "gzip, deflate"},       ///< 16
                {"accept-language", ""},                    ///< 17
                {"accept-ranges", ""},                      ///< 18
                {"accept", ""},                             ///< 19
                {"access-control-allow-origin", ""},        ///< 20
                {"age", ""},                                ///< 21
                {"allow", ""},                              ///< 22
                {"authorization", ""},                      ///< 23
                {"cache-control", ""},                      ///< 24
                {"content-disposition", ""},                ///< 25
                {"content-encoding", ""},                   ///< 26
                {"content-language", ""},                   ///< 27
                {"content-length", ""},                     ///< 28
                {"content-location", ""},                   ///< 29
                {"content-range", ""},                      ///< 30
                {"content-type", ""},                       ///< 31
                {"cookie", ""},                             ///< 32
                {"date", ""},                               ///< 33
                {"etag", ""},                               ///< 34
                {"expect", ""},                             ///< 35
                {"expires", ""},                            ///< 36
                {"from", ""},                               ///< 37
                {"host", ""},                               ///< 38
                {"if-match", ""},                           ///< 39
                {"if-modified-since", ""},                  ///< 40
                {"if-none-match", ""},                      ///< 41
                {"if-range", ""},                           ///< 42
                {"if-unmodified-since", ""},                ///< 43
                {"last-modified", ""},                      ///< 44
                {"link", ""},                               ///< 45
                {"location", ""},                           ///< 46
                {"max-forwards", ""},                       ///< 47
                {"proxy-authenticate", ""},                 ///< 48
                {"proxy-authorization", ""},                ///< 49
                {"range", ""},                              ///< 50
                {"referer", ""},                            ///< 51
                {"refresh", ""},                            ///< 52
                {"retry-after", ""},                        ///< 53
                {"server", ""},                             ///< 54
                {"set-cookie", ""},                         ///< 55
                {"strict-transport-security", ""},          ///< 56
                {"transfer-encoding", ""},                  ///< 57
                {"user-agent", ""},                         ///< 58
                {"vary", ""},                               ///< 59
                {"via", ""},                                ///< 60
                {"www-authenticate", ""}                    ///< 61
        };

    static const std::unordered_map<std::string, size_t> sc_mapStaticTable = {
            {":authority", 1},
            {":method GET", 2},
            {":method POST", 3},
            {":path /", 4},
            {":path /index.html", 5},
            {":scheme http", 6},
            {":scheme https", 7},
            {":status 200", 8},
            {":status 204", 9},
            {":status 206", 10},
            {":status 304", 11},
            {":status 400", 12},
            {":status 404", 13},
            {":status 500", 14},
            {"accept-charset", 15},
            {"accept-encoding gzip, deflate", 16},
            {"accept-language", 17},
            {"accept-ranges", 18},
            {"accept", 19},
            {"access-control-allow-origin", 20},
            {"age", 21},
            {"allow", 22},
            {"authorization", 23},
            {"cache-control", 24},
            {"content-disposition", 25},
            {"content-encoding", 26},
            {"content-language", 27},
            {"content-length", 28},
            {"content-location", 29},
            {"content-range", 30},
            {"content-type", 31},
            {"cookie", 32},
            {"date", 33},
            {"etag", 34},
            {"expect", 35},
            {"expires", 36},
            {"from", 37},
            {"host", 38},
            {"if-match", 39},
            {"if-modified-since", 40},
            {"if-none-match", 41},
            {"if-range", 42},
            {"if-unmodified-since", 43},
            {"last-modified", 44},
            {"link", 45},
            {"location", 46},
            {"max-forwards", 47},
            {"proxy-authenticate", 48},
            {"proxy-authorization", 49},
            {"range", 50},
            {"referer", 51},
            {"refresh", 52},
            {"retry-after", 53},
            {"server", 54},
            {"set-cookie", 55},
            {"strict-transport-security", 56},
            {"transfer-encoding", 57},
            {"user-agent", 58},
            {"vary", 59},
            {"via", 60},
            {"www-authenticate", 61}
    };
};

} /* namespace neb */

#endif /* SRC_CODEC_HTTP2_HTTP2HEADER_HPP_ */

