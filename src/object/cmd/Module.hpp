/*******************************************************************************
 * Project:  Nebula
 * @file     Module.hpp
 * @brief    Http消息入口
 * @author   Bwar
 * @date:    2016年8月12日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_CMD_MODULE_HPP_
#define SRC_OBJECT_CMD_MODULE_HPP_

#include "codec/CodecHttp.hpp"
#include "object/Object.hpp"

namespace neb
{

class Worker;

class Module: public Object
{
public:
    Module();
    virtual ~Module();

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
    const std::string& GetModulePath() const
    {
        return(m_strModulePath);
    }

private:
    void SetModulePath(const std::string& strModulePaht)
    {
        m_strModulePath = strModulePaht;
    }

private:
    std::string m_strModulePath;

    friend class Worker;
};

} /* namespace neb */

#endif /* SRC_OBJECT_CMD_MODULE_HPP_ */
