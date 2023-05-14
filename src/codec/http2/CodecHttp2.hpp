/*******************************************************************************
 * Project:  Nebula
 * @file     CodecHttp2.hpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-01
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_HTTP2_CODECHTTP2_HPP_
#define SRC_CODEC_HTTP2_CODECHTTP2_HPP_

#include <unordered_map>
#include "codec/Codec.hpp"
#include "util/http/http_parser.h"
#include "pb/http.pb.h"
#include "H2Comm.hpp"
#include "Tree.hpp"
#include "Http2Header.hpp"
#include "channel/SpecChannel.hpp"
#include "labor/LaborShared.hpp"

namespace neb
{

const uint32 STREAM_IDENTIFY_MASK = 0x7FFFFFFF;

struct tagStreamWeight
{
    uint32 uiStreamId                       = 0;
    uint8 ucWeight                          = 0;
    uint8 ucCurrentWeight                   = 0;
};

class Http2Frame;
class Http2Stream;

class CodecHttp2: public Codec
{
public:
    CodecHttp2(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, std::shared_ptr<SocketChannel> pBindChannel);
    virtual ~CodecHttp2();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_HTTP2);
    }

    // request
    static int Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, const HttpMsg& oHttpMsg);

    // response
    static int Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, const HttpMsg& oHttpMsg);

    virtual bool DecodeWithReactor() const
    {
        return(true);
    }

    E_CODEC_STATUS Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff = nullptr);
    E_CODEC_STATUS Encode(const HttpMsg& oHttpMsg, CBuffer* pBuff);
    E_CODEC_STATUS Encode(const HttpMsg& oHttpMsg, CBuffer* pBuff, CBuffer* pSecondlyBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, HttpMsg& oHttpMsg);
    E_CODEC_STATUS Decode(CBuffer* pBuff, HttpMsg& oHttpMsg, CBuffer* pReactBuff);

    /**
     * @brief HTTP2 连接建立
     */
    virtual void ConnectionSetting(CBuffer* pBuff);

public:
    inline bool IsEstablish() const
    {
        if (m_bClientPrefaceMagic && m_bServerPreface)
        {
            return(true);
        }
        return(false);
    }
    void SetPriority(uint32 uiStreamId, const tagPriority& stPriority);
    void RstStream(uint32 uiStreamId);
    E_H2_ERR_CODE Setting(const std::vector<tagSetting>& vecSetting, bool bRecvSetting = false);
    bool WindowUpdate(uint32 uiStreamId, uint32 uiIncrement);
    void UpdateSendWindow(uint32 uiStreamId, uint32 uiSendLength);
    void UpdateRecvWindow(uint32 uiStreamId, uint32 uiRecvLength, CBuffer* pBuff);
    int32 GetSendWindowSize()
    {
        return(m_iSendWindowSize);
    }
    uint32 GetMaxEncodeFrameSize()
    {
        return(m_uiSettingsMaxEncodeFrameSize);
    }
    E_CODEC_STATUS UnpackHeader(uint32 uiHeaderBlockEndPos, CBuffer* pBuff, HttpMsg& oHttpMsg);
    void PackHeader(const HttpMsg& oHttpMsg, int iHeaderType, CBuffer* pBuff);
    void PackHeader(const std::string& strHeaderName, const std::string& strHeaderValue, bool bWithHuffman, CBuffer* pBuff);
    E_CODEC_STATUS PromiseStream(uint32 uiStreamId, CBuffer* pReactBuff);
    void SetGoaway(uint32 uiLastStreamId)
    {
        m_uiGoawayLastStreamId = uiLastStreamId;
    }
    uint32 GetLastStreamId()
    {
        return(m_uiStreamIdGenerate);
    }

    E_CODEC_STATUS SendWaittingFrameData(CBuffer* pBuff);
    E_CODEC_STATUS SendWaittingFrameData(TreeNode<tagStreamWeight>* pStreamWeightNode,
            std::vector<uint32>& vecCompletedStream, CBuffer* pBuff);
    void TransferHoldingMsg(HttpMsg* pHoldingHttpMsg);
    void SetWaittingFrame(bool bHasWaittingFrame)
    {
        m_bHasWaittingFrame = bHasWaittingFrame;
    }

    void DebugDecodingTable();

protected:
    uint32 StreamIdGenerate();
    Http2Stream* NewCodingStream(uint32 uiStreamId);
    TreeNode<tagStreamWeight>* FindStreamWeight(uint32 uiStreamId, TreeNode<tagStreamWeight>* pTarget);
    TreeNode<tagStreamWeight>* FindHighestWeightStream();
    void ReleaseStreamWeight(TreeNode<tagStreamWeight>* pNode);

    E_CODEC_STATUS UnpackHeaderIndexed(CBuffer* pBuff, HttpMsg& oHttpMsg);
    E_CODEC_STATUS UnpackHeaderLiteralIndexing(CBuffer* pBuff, uint8 ucFirstByte, int32 iPrefixMask,
            std::string& strHeaderName, std::string& strHeaderValue, bool& bWithHuffman);
    void ClassifyHeader(const std::string& strHeaderName, const std::string& strHeaderValue, HttpMsg& oHttpMsg);
    void PackHeaderIndexed(size_t uiTableIndex, CBuffer* pBuff);
    void PackHeaderWithIndexing(const std::string& strHeaderName,
            const std::string& strHeaderValue, uint32 uiHeaderNameIndex, bool bWithHuffman, CBuffer* pBuff);
    void PackHeaderWithoutIndexing(const std::string& strHeaderName,
            const std::string& strHeaderValue, uint32 uiHeaderNameIndex, bool bWithHuffman, CBuffer* pBuff);
    void PackHeaderNeverIndexing(const std::string& strHeaderName,
            const std::string& strHeaderValue, uint32 uiHeaderNameIndex, bool bWithHuffman, CBuffer* pBuff);
    void PackHeaderDynamicTableSize(uint32 uiDynamicTableSize, CBuffer* pBuff);
    uint32 GetEncodingTableIndex(const std::string& strHeaderName, const std::string& strHeaderValue, uint32& uiNameIndex);
    uint32 GetDecodingTableIndex(const std::string& strHeaderName, const std::string& strHeaderValue, uint32& uiNameIndex);
    void UpdateEncodingDynamicTable(const std::string& strHeaderName, const std::string& strHeaderValue);
    uint32 UpdateEncodingDynamicTable(int32 iRecoverSize);
    uint32 EncodingDynamicTableIndex2VectorIndex(uint32 uiIndex);
    void UpdateDecodingDynamicTable(int32 iVectorIndex, const std::string& strHeaderName, const std::string& strHeaderValue);
    uint32 UpdateDecodingDynamicTable(int32 iRecoverSize);
    uint32 DecodingDynamicTableIndex2VectorIndex(uint32 uiIndex);
    void CloseStream(uint32 uiStreamId);

private:
    bool m_bClientPrefaceMagic = false;
    bool m_bServerPreface = false;
    bool m_bHasWaittingFrame = false;
    uint32 m_uiStreamIdGenerate = 0;
    uint32 m_uiGoawayLastStreamId = 0;
    uint32 m_uiSettingsEnablePush = 1;
    uint32 m_uiSettingsHeaderTableSize = 4096;
    uint32 m_uiSettingsMaxConcurrentStreams = 100;
    int32 m_iSettingsMaxSendWindowSize = DEFAULT_SETTINGS_MAX_INITIAL_WINDOW_SIZE;
    int32 m_iSettingsMaxRecvWindowSize = DEFAULT_SETTINGS_MAX_INITIAL_WINDOW_SIZE;
    uint32 m_uiSettingsMaxEncodeFrameSize = DEFAULT_SETTINGS_MAX_FRAME_SIZE;
    uint32 m_uiSettingsMaxDecodeFrameSize = DEFAULT_SETTINGS_MAX_FRAME_SIZE;
    uint32 m_uiSettingsMaxHeaderListSize = 8192;  // TODO SettingsMaxHeaderListSize
    int32 m_iSendWindowSize = DEFAULT_SETTINGS_MAX_INITIAL_WINDOW_SIZE;
    int32 m_iRecvWindowSize = DEFAULT_SETTINGS_MAX_INITIAL_WINDOW_SIZE;
    tagH2FrameHead m_stDecodeFrameHead;
    HttpMsg* m_pHoldingHttpMsg = nullptr;       // upgrade未完成时暂存请求
    Http2Frame* m_pFrame = nullptr;
    Http2Stream* m_pCodingStream = nullptr;
    std::unordered_map<uint32, Http2Stream*> m_mapStream;
    TreeNode<tagStreamWeight>* m_pStreamWeightRoot = nullptr;

    std::vector<Http2Header> m_vecEncodingDynamicTable;
    uint32 m_uiEncodingDynamicTableSize = 0;
    uint32 m_uiEncodingDynamicTableHeaderCount = 0;
    int32 m_iNextEncodingDynamicTableHeaderIndex = 0;
    std::vector<Http2Header> m_vecDecodingDynamicTable;
    uint32 m_uiDecodingDynamicTableSize = 0;
    uint32 m_uiDecodingDynamicTableHeaderCount = 0;
    int32 m_iNextDecodingDynamicTableHeaderIndex = 0;
    std::unordered_set<std::string> m_setEncodingWithoutIndexHeaders;
    std::unordered_set<std::string> m_setEncodingNeverIndexHeaders;
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECHTTP2_HPP_ */

