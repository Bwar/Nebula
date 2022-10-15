/*******************************************************************************
 * Project:  Nebula
 * @file     CodecCass.hpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-28
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CASSANDRA_CODECCASS_HPP_
#define SRC_CODEC_CASSANDRA_CODECCASS_HPP_

#include <unordered_set>
#include "codec/Codec.hpp"
#include "CassMessage.hpp"

namespace neb
{

/**
 * @brief
 * @note
 * The CQL binary protocol is a frame based protocol. Frames are defined as:
 *
 *    0         8        16        24        32         40
 *    +---------+---------+---------+---------+---------+
 *    | version |  flags  |      stream       | opcode  |
 *    +---------+---------+---------+---------+---------+
 *    |                length                 |
 *    +---------+---------+---------+---------+
 *    |                                       |
 *    .            ...  body ...              .
 *    .                                       .
 *    .                                       .
 *    +----------------------------------------
 *
 * The protocol is big-endian (network byte order).
 */
class CodecCass: public Codec
{
public:
    CodecCass(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
            std::shared_ptr<SocketChannel> pBindChannel);
    virtual ~CodecCass();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_CASS);
    }

    virtual bool DecodeWithReactor() const
    {
        return(true);
    }

    E_CODEC_STATUS Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff = nullptr);
    E_CODEC_STATUS Encode(const CassMessage& oCassMsg, CBuffer* pBuff);
    E_CODEC_STATUS Encode(const CassMessage& oCassMsg, CBuffer* pBuff, CBuffer* pSecondlyBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, CassMessage& oCassMsg);
    E_CODEC_STATUS Decode(CBuffer* pBuff, CassMessage& oCassMsg, CBuffer* pReactBuff);

    uint16 GetLastStreamId() const
    {
        return(m_unLastReqStreamId);
    }

    static void DecodeFrameHeader(CBuffer* pBuff, CassFrameHead& stFrameHead);
    static void EncodeFrameHeader(const CassFrameHead& stFrameHead, CBuffer* pBuff);

protected:
    uint16 GenerateStreamId();
    void Ready();

private:
    uint8 m_ucVersion = 4;
    uint16 m_unLastReqStreamId = 0;
    bool m_bIsReady = false;
    bool m_bStartup = false;
    std::unordered_set<uint16> m_setReqStreamId;
    CBuffer m_oReqBuff;
};

} /* namespace neb */

#endif /* SRC_CODEC_CASSANDRA_CODECCASS_HPP_ */

