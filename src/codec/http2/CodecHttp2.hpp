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
#include "pb/http.pb.h"
#include "H2Comm.hpp"
#include "Tree.hpp"
#include "Http2Header.hpp"

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
    CodecHttp2(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType);
    virtual ~CodecHttp2();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff)
    {
        return(CODEC_STATUS_INVALID);
    }
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
    {
        return(CODEC_STATUS_INVALID);
    }

    virtual E_CODEC_STATUS Encode(const HttpMsg& oHttpMsg, CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, HttpMsg& oHttpMsg, CBuffer* pReactBuff);

public:
    bool IsChunkNotice() const
    {
        return(m_bChunkNotice);
    }
    void SetPriority(uint32 uiStreamId, const tagPriority& stPriority);
    void RstStream(uint32 uiStreamId);
    E_H2_ERR_CODE Setting(const std::vector<tagSetting>& vecSetting);
    void WindowUpdate(uint32 uiStreamId, uint32 uiIncrement);
    uint32 GetMaxFrameSize()
    {
        return(m_uiSettingsMaxFrameSize);
    }
    E_CODEC_STATUS UnpackHeader(uint32 uiHeaderBlockEndPos, CBuffer* pBuff, HttpMsg& oHttpMsg);
    void PackHeader(const HttpMsg& oHttpMsg, CBuffer* pBuff);
    E_CODEC_STATUS PromiseStream(uint32 uiStreamId, CBuffer* pReactBuff);
    void SetGoaway(uint32 uiLastStreamId)
    {
        m_uiGoawayLastStreamId = uiLastStreamId;
    }
    uint32 GetLastStreamId()
    {
        return(m_uiStreamIdGenerate);
    }

protected:
    uint32 StreamIdGenerate();
    TreeNode<tagStreamWeight>* FindStreamWeight(uint32 uiStreamId, TreeNode<tagStreamWeight>* pTarget);
    TreeNode<tagStreamWeight>* FindHighestWeightStream();
    void ReleaseStreamWeight(TreeNode<tagStreamWeight>* pNode);

    E_CODEC_STATUS UnpackHeaderIndexed(CBuffer* pBuff,
            ::google::protobuf::Map< ::std::string, ::std::string >* pHeader);
    E_CODEC_STATUS UnpackHeaderLiteralIndexing(CBuffer* pBuff, uint8 ucFirstByte, int32 iPrefixMask,
            int& iDynamicTableIndex, std::string& strHeaderName, std::string& strHeaderValue,
            bool& bWithHuffman);
    void PackHeaderIndexed(size_t uiTableIndex, CBuffer* pBuff);
    void PackHeaderWithIndexing(const std::string& strHeaderName,
            const std::string& strHeaderValue, bool bWithHuffman, CBuffer* pBuff);
    void PackHeaderWithoutIndexing(const std::string& strHeaderName,
            const std::string& strHeaderValue, bool bWithHuffman, CBuffer* pBuff);
    void PackHeaderNeverIndexing(const std::string& strHeaderName,
            const std::string& strHeaderValue, bool bWithHuffman, CBuffer* pBuff);
    void PackHeaderDynamicTableSize(uint32 uiDynamicTableSize, CBuffer* pBuff);
    size_t GetEncodingTableIndex(const std::string& strHeaderName, const std::string& strHeaderValue = "");
//    size_t GetDecodingTableIndex(const std::string& strHeaderName, const std::string& strHeaderValue = "");
    void UpdateEncodingDynamicTable(int iDynamicTableIndex, const std::string& strHeaderName, const std::string& strHeaderValue);
    void UpdateEncodingDynamicTable(uint32 uiTableSize);
    void UpdateDecodingDynamicTable(int iDynamicTableIndex, const std::string& strHeaderName, const std::string& strHeaderValue);
    void UpdateDecodingDynamicTable(uint32 uiTableSize);

private:
    bool m_bChannelIsClient = false;    // 当前编解码器所在channel是作为http客户端还是作为http服务端
    bool m_bChunkNotice = false;        // 是否启用分块传输通知（当包体比较大时，部分传输完毕也会通知业务层而无须等待整个http包传输完毕。）
    uint32 m_uiStreamIdGenerate = 0;
    uint32 m_uiGoawayLastStreamId = 0;
    uint32 m_uiSettingsEnablePush = 1;
    uint32 m_uiSettingsHeaderTableSize = 4096;
    uint32 m_uiSettingsMaxConcurrentStreams = 100;
    uint32 m_uiSettingsMaxWindowSize = DEFAULT_SETTINGS_MAX_INITIAL_WINDOW_SIZE;
    uint32 m_uiSettingsMaxFrameSize = DEFAULT_SETTINGS_MAX_FRAME_SIZE;
    uint32 m_uiSettingsMaxHeaderListSize = 50;  // TODO SettingsMaxHeaderListSize
    tagH2FrameHead m_stFrameHead;
    std::unique_ptr<Http2Frame> m_pFrame = nullptr;
    Http2Stream* m_pCodingStream = nullptr;
    std::unordered_map<uint32, Http2Stream*> m_mapStream;
    TreeNode<tagStreamWeight>* m_pStreamWeight = nullptr;

    uint32 m_uiEncodingDynamicTableSize = 0;
    uint32 m_uiDecodingDynamicTableSize = 0;
    std::vector<Http2Header> m_vecEncodingDynamicTable;
    std::vector<Http2Header> m_vecDecodingDynamicTable;
    std::unordered_set<std::string> m_setEncodingWithoutIndexHeaders;
    std::unordered_set<std::string> m_setEncodingNeverIndexHeaders;
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECHTTP2_HPP_ */

