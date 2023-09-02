/*******************************************************************************
 * Project:  Nebula
 * @file     CassComm.hpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-27
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CASSANDRA_CASSCOMM_HPP_
#define SRC_CODEC_CASSANDRA_CASSCOMM_HPP_

#include "Definition.hpp"

namespace neb
{

/**
 * @brief request or response
 * @note a single byte that indicates both the direction of the message
 * (request or response) and the version of the protocol in use.
 */
enum E_CASS_STREAM_TYPE
{
    CASS_MSG_REQUEST                = 0x00,
    CASS_MSG_RESPONSE               = 0x80,
};

/**
 * @brief
 * @note The version is a single byte that indicates both the direction of
 * the message (request or response) and the version of the protocol in use.
 * The most significant bit of version is used to define the direction of
 * the message: 0 indicates a request, 1 indicates a response.
 */
enum E_CASS_VERSION
{
    CASS_VERSION_BITS               = 0x7F,
};

/**
 * @brief flags
 * @note Flags applying to this frame. The flags have the following
 * meaning (described by the mask that allows selecting them):
 */
enum E_CASS_FLAGS
{
    /**
     * @note Compression flag. If set, the frame body is compressed. The actual
     * compression to use should have been set up beforehand through the
     * Startup message (which thus cannot be compressed; Section 4.1.1).
     */
    CASS_FLAG_COMPRESSION           = 0x01,
    /**
     * @note Tracing flag. For a request frame, this indicates the client requires
     * tracing of the request. Note that only QUERY, PREPARE and EXECUTE queries
     * support tracing. Other requests will simply ignore the tracing flag if
     * set. If a request supports tracing and the tracing flag is set, the response
     * to this request will have the tracing flag set and contain tracing information.
     * If a response frame has the tracing flag set, its body contains
     * a tracing ID. The tracing ID is a [uuid] and is the first thing in
     * the frame body.
     */
    CASS_FLAG_TRACING               = 0x02,
    /**
     * @note Custom payload flag. For a request or response frame, this indicates
     * that a generic key-value custom payload for a custom QueryHandler
     * implementation is present in the frame. Such a custom payload is simply
     * ignored by the default QueryHandler implementation.
     * Currently, only QUERY, PREPARE, EXECUTE and BATCH requests support payload.
     * Type of custom payload is [bytes map] (see below). If either or both
     * of the tracing and warning flags are set, the custom payload will follow
     * those indicated elements in the frame body. If neither are set, the custom
     * payload will be the first value in the frame body.
     */
    CASS_FLAG_CUSTOM_PAYLOAD        = 0x04,
    /**
     * @note Warning flag. The response contains warnings which were generated
     * by the server to go along with this response.
     * If a response frame has the warning flag set, its body will contain the
     * text of the warnings. The warnings are a [string list] and will be the
     * first value in the frame body if the tracing flag is not set, or directly
     * after the tracing ID if it is.
     */
    CASS_FLAG_WARNING               = 0x08,

    // The rest of flags is currently unused and ignored.
};

enum E_CASS_OP_CODE
{
    CASS_OP_ERROR                   = 0x00,     ///<
    CASS_OP_STARTUP                 = 0x01,
    CASS_OP_READY                   = 0x02,
    CASS_OP_AUTHENTICATE            = 0x03,
    CASS_OP_OPTIONS                 = 0x05,
    CASS_OP_SUPPORTED               = 0x06,
    CASS_OP_QUERY                   = 0x07,
    CASS_OP_RESULT                  = 0x08,
    CASS_OP_PREPARE                 = 0x09,
    CASS_OP_EXECUTE                 = 0x0A,
    CASS_OP_REGISTER                = 0x0B,
    CASS_OP_EVENT                   = 0x0C,
    CASS_OP_BATCH                   = 0x0D,
    CASS_OP_AUTH_CHALLENGE          = 0x0E,
    CASS_OP_AUTH_RESPONSE           = 0x0F,
    CASS_OP_AUTH_SUCCESS            = 0x10
};

/**
 * @brief cassandra error code
 * @note an ERROR message is composed of <code><message>[...]
 * (see 4.2.1 for details). The supported error codes, as well as any additional
 * information the message may contain after the <message> are described below:
 */
enum E_CASS_ERROR_CODE
{
    CASS_ERR_SERVER                 = 0x0000,   ///< something unexpected happened. This indicates a server-side bug.
    CASS_ERR_PROTOCOL               = 0x000A,   ///< some client message triggered a protocol violation (for instance a QUERY message is sent before a STARTUP one has been sent)
    CASS_ERR_AUTHENTICATION         = 0x0100,   ///< authentication was required and failed. The possible reason for failing depends on the authenticator in use, which may or may not include more detail in the accompanying error message.
    CASS_ERR_UNAVAILABLE            = 0x1000,   ///< Unavailable exception. The rest of the ERROR message body will be <cl><required><alive>
    CASS_ERR_OVERLOADED             = 0x1001,   ///< the request cannot be processed because the coordinator node is overloaded
    CASS_ERR_BOOTSTRAPPING          = 0x1002,   ///< the request was a read request but the coordinator node is bootstrapping
    CASS_ERR_TRUNCATE               = 0x1003,   ///< error during a truncation error.
    CASS_ERR_WRITE_TIMEOUT          = 0x1100,   ///< Timeout exception during a write request. The rest of the ERROR message body will be <cl><received><blockfor><writeType>
    CASS_ERR_READ_TIMEOUT           = 0x1200,   ///< Timeout exception during a read request. The rest of the ERROR message body will be <cl><received><blockfor><data_present>
    CASS_ERR_READ_FAILURE           = 0x1300,   ///< A non-timeout exception during a read request. The rest of the ERROR message body will be <cl><received><blockfor><numfailures><data_present>
    CASS_ERR_FUNCTION_FAILURE       = 0x1400,   ///< A (user defined) function failed during execution. The rest of the ERROR message body will be <keyspace><function><arg_types>
    CASS_ERR_WRITE_FAILURE          = 0x1500,   ///< A non-timeout exception during a write request. The rest of the ERROR message body will be <cl><received><blockfor><numfailures><write_type>
    CASS_ERR_SYNTAX                 = 0x2000,   ///< The submitted query has a syntax error.
    CASS_ERR_UNAUTHORIZED           = 0x2100,   ///< The logged user doesn't have the right to perform the query.
    CASS_ERR_INVALID                = 0x2200,   ///< The query is syntactically correct but invalid.
    CASS_ERR_CONFIG                 = 0x2300,   ///< The query is invalid because of some configuration issue
    CASS_ERR_ALREADY_EXIST          = 0x2400,   ///< The query attempted to create a keyspace or a table that was already existing. The rest of the ERROR message body will be <ks><table>
    CASS_ERR_UNPREPARED             = 0x2500,   ///< Can be thrown while a prepared statement tries to be executed if the provided prepared statement ID is not known by this host. The rest of the ERROR message body will be [short bytes] representing the unknown ID.
};

/*
 * @brief A consistency level specification.
 */
enum E_CASS_CONSISTENCY
{
    CASS_CONSIST_ANY                = 0x0000,
    CASS_CONSIST_ONE                = 0x0001,
    CASS_CONSIST_TWO                = 0x0002,
    CASS_CONSIST_THREE              = 0x0003,
    CASS_CONSIST_QUORUM             = 0x0004,
    CASS_CONSIST_ALL                = 0x0005,
    CASS_CONSIST_LOCAL_QUORUM       = 0x0006,
    CASS_CONSIST_EACH_QUORUM        = 0x0007,
    CASS_CONSIST_SERIAL             = 0x0008,
    CASS_CONSIST_LOCAL_SERIAL       = 0x0009,
    CASS_CONSIST_LOCAL_ONE          = 0x000A,
};

/*
 * @brief The CQL binary protocol is a frame based protocol.
 * @note The protocol is big-endian (network byte order). Each frame contains
 * a fixed size header (9 bytes) followed by a variable size.
  body.
 */
struct CassFrameHead
{
    uint8 ucVersion                 = 0; ///< The version is a single byte that indicates both the direction of the message(request or response) and the version of the protocol in use.
    uint8 ucFlags                   = 0; ///< Flags applying to this frame. The flags have the meaning: E_CASS_FLAGS
    uint8 ucOpCode                  = 0; ///< An integer byte that distinguishes the actual message: E_CASS_OP_CODE
    uint16 uiStream                 = 0; ///< A frame has a stream id (a [short] value). When sending request messages, this stream id must be set by the client to a non-negative value (negative stream id are reserved for streams initiated by the server; currently all EVENT messages(section 4.2.6) have a streamId of -1). If a client sends a request message with the stream id X, it is guaranteed that the stream id of the response to that message will be X.
    uint32 uiLength                 = 0; ///< A 4 byte integer representing the length of the body of the frame (note: currently a frame is limited to 256MB in length).
    CassFrameHead& operator=(const CassFrameHead& stFrameHead)
    {
        ucVersion = stFrameHead.ucVersion;
        ucFlags = stFrameHead.ucFlags;
        ucOpCode = stFrameHead.ucOpCode;
        uiStream = stFrameHead.uiStream;
        uiLength = stFrameHead.uiLength;
        return(*this);
    }
};

}

#endif /* SRC_CODEC_CASSANDRA_CASSCOMM_HPP_ */

