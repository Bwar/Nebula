/*******************************************************************************
 * Project:  Nebula
 * @file     PbContext.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年6月21日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_PBCONTEXT_HPP_
#define SRC_ACTOR_PBCONTEXT_HPP_

#include "actor/DynamicCreator.hpp"
#include "Context.hpp"

namespace neb
{

class ActorBuilder;

class PbContext: public Context
{
public:
    PbContext();
    PbContext(
            std::shared_ptr<SocketChannel> pChannel,
            int32 iCmd, uint32 uiSeq);
    PbContext(
            std::shared_ptr<SocketChannel> pChannel,
            int32 iCmd, uint32 uiSeq,
            const MsgBody& oReqMsgBody);
    PbContext(const PbContext&) = delete;
    PbContext& operator=(const PbContext&) = delete;
    virtual ~PbContext();

    virtual void Done()
    {
    }

public:
    /**
     * @brief 给请求方发响应
     * @param iErrno 错误码，如果正确则应传入ERR_OK
     * @param strData 响应结果（错误码为ERR_OK时），或错误信息
     */
    bool Response(int iErrno, const std::string& strData);

    int32 GetCmd() const
    {
        return(m_iReqCmd);
    }

    uint32 GetSeq() const
    {
        return(m_uiReqSeq);
    }

    const MsgBody& GetMsgBody() const
    {
        return(m_oReqMsgBody);
    }

private:
    int32 m_iReqCmd;
    uint32 m_uiReqSeq;
    MsgBody m_oReqMsgBody;
    
    friend class ActorBuilder;
};

} /* namespace neb */

#endif /* SRC_ACTOR_PBCONTEXT_HPP_ */
