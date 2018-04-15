/*******************************************************************************
 * Project:  Nebula
 * @file     Module.hpp
 * @brief    Http消息入口
 * @author   Bwar
 * @date:    2016年8月12日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_MODULE_HPP_
#define SRC_ACTOR_CMD_MODULE_HPP_

#include "codec/CodecHttp.hpp"
#include "labor/Worker.hpp"
#include "ModuleModel.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{
class Module: public ModuleModel
{
public:
    Module(const std::string& strModulePath)
        : ModuleModel(strModulePath)
    {
    }
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;
    virtual ~Module(){};

    /**
     * @brief 初始化Module
     * @note Module类实例初始化函数，大部分Module不需要初始化，需要初始化的Module可派生后实现此函数，
     * 在此函数里可以读取配置文件（配置文件必须为json格式）。配置文件由Module的设计者自行定义，
     * 存放于conf/目录，配置文件名最好与Module名字保持一致，加上.json后缀。配置文件的更新同步
     * 会由框架自动完成。
     * @return 是否初始化成功
     */
    virtual bool Init()
    {
        return(true);
    }

    /**
     * @brief http服务模块处理入口
     * @param stCtx 消息来源上下文
     * @param oHttpMsg 接收到的http数据包
     * @return 是否处理成功
     */
    virtual bool AnyMessage(
                    const tagChannelContext& stCtx,
                    const HttpMsg& oHttpMsg) = 0;

protected:
    template <typename ...Targs> void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args);
    template <typename ...Targs> std::shared_ptr<Step> MakeSharedStep(const std::string& strStepName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Session> MakeSharedSession(const std::string& strSessionName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Cmd> MakeSharedCmd(const std::string& strCmdName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Module> MakeSharedModule(const std::string& strModuleName, Targs... args);
};

template <typename ...Targs>
void Module::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args)
{
    m_pWorker->Logger(m_strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Step> Module::MakeSharedStep(const std::string& strStepName, Targs... args)
{
    return(m_pWorker->NewStep(this, strStepName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Session> Module::MakeSharedSession(const std::string& strSessionName, Targs... args)
{
    return(m_pWorker->NewSession(this, strSessionName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Cmd> Module::MakeSharedCmd(const std::string& strCmdName, Targs... args)
{
    return(m_pWorker->NewStep(this, strCmdName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Module> Module::MakeSharedModule(const std::string& strModuleName, Targs... args)
{
    return(m_pWorker->NewSession(this, strModuleName, std::forward<Targs>(args)...));
}

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_MODULE_HPP_ */
