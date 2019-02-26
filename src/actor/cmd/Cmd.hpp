/*******************************************************************************
 * Project:  Nebula
 * @file     Cmd.hpp
 * @brief    业务处理入口
 * @author   Bwar
 * @date:    2016年8月12日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_CMD_HPP_
#define SRC_ACTOR_CMD_CMD_HPP_

#include "labor/Worker.hpp"
#include "CmdModel.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class Cmd: public CmdModel
{
public:
    Cmd(int32 iCmd)
        : CmdModel(iCmd)
    {
    }
    Cmd(const Cmd&) = delete;
    Cmd& operator=(const Cmd&) = delete;
    virtual ~Cmd(){};

    /**
     * @brief 初始化Cmd
     * @note Cmd类实例初始化函数，大部分Cmd不需要初始化，需要初始化的Cmd可派生后实现此函数，
     * 在此函数里可以读取配置文件（配置文件必须为json格式）。配置文件由Cmd的设计者自行定义，
     * 存放于conf/目录，配置文件名最好与Cmd名字保持一致，加上.json后缀。配置文件的更新同步
     * 会由框架自动完成。
     * @return 是否初始化成功
     */
    virtual bool Init()
    {
        return(true);
    }

    /**
     * @brief 命令处理入口
     * @note 框架层成功解析数据包后，根据MsgHead里的Cmd找到对应的Cmd类实例调用将数据包及
     * 数据包来源tagChannelContext传给AnyMessage处理。若处理过程不涉及网络IO之类需异步处
     * 理的耗时调用，则无需新创建Step类实例来处理。若处理过程涉及耗时异步调用，则应创建Step
     * 类实例，并向框架层注册Step类实例，调用Step.Start()后即返回。
     * @param stCtx 消息来源上下文
     * @param oMsgHead 数据包头
     * @param oMsgBody 数据包体
     * @return 命令是否处理成功
     */
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody) = 0;

protected:
    template <typename ...Targs> void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args);
    template <typename ...Targs> std::shared_ptr<Step> MakeSharedStep(const std::string& strStepName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Session> MakeSharedSession(const std::string& strSessionName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Cmd> MakeSharedCmd(const std::string& strCmdName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Module> MakeSharedModule(const std::string& strModuleName, Targs... args);
};

template <typename ...Targs>
void Cmd::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args)
{
    m_pWorker->Logger(m_strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Step> Cmd::MakeSharedStep(const std::string& strStepName, Targs... args)
{
    return(m_pWorker->MakeSharedStep(this, strStepName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Session> Cmd::MakeSharedSession(const std::string& strSessionName, Targs... args)
{
    return(m_pWorker->MakeSharedSession(this, strSessionName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Cmd> Cmd::MakeSharedCmd(const std::string& strCmdName, Targs... args)
{
    return(m_pWorker->MakeSharedStep(this, strCmdName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Module> Cmd::MakeSharedModule(const std::string& strModuleName, Targs... args)
{
    return(m_pWorker->MakeSharedSession(this, strModuleName, std::forward<Targs>(args)...));
}

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_CMD_HPP_ */
