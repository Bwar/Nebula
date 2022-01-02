/*******************************************************************************
 * Project:  Nebula
 * @file     CassMessage.hpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-27
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CASSANDRA_CASSMESSAGE_HPP_
#define SRC_CODEC_CASSANDRA_CASSMESSAGE_HPP_

#include "CassComm.hpp"
#include "type/Notations.hpp"

namespace neb
{

class CodecCass;

/**
 * @note Dependant on the flags specified in the header, the layout of the message body must be:
 * [<tracing_id>][<warnings>][<custom_payload>]<message>
 * where:
 *   - <tracing_id> is a UUID tracing ID, present if this is a request message and the Tracing flag is set.
 *   - <warnings> is a string list of warnings (if this is a request message and the Warning flag is set.
 *   - <custom_payload> is bytes map for the serialised custom payload present if this is one of the message types
 *     which support custom payloads (QUERY, PREPARE, EXECUTE and BATCH) and the Custom payload flag is set.
 *   - <message> as defined below through sections 4 and 5.
 */
class CassMessage
{
public:
    CassMessage(uint16 uiMsgType = CASS_MSG_REQUEST);
    CassMessage(const CassMessage&) = delete;
    virtual ~CassMessage();

    CassMessage& operator=(const CassMessage&) = delete;

    uint16 GetMsgType() const
    {
        return(m_unMsgType);
    }

    uint16 GetStreamId() const
    {
        return(m_unStreamId);
    }

    uint8 GetOpcode() const
    {
        return(m_ucOpcode);
    }

    void SetOpcode(uint8 ucOpcode)
    {
        m_ucOpcode = ucOpcode;
    }

private:
    friend class CodecCass;
    uint8 m_ucOpcode;
    uint16 m_unMsgType;
    uint16 m_unStreamId;
};

} /* namespace neb */

#endif /* SRC_CODEC_CASSANDRA_CASSMESSAGE_HPP_ */

