/*******************************************************************************
 * Project:  Nebula
 * @file     ModuleModel.hpp
 * @brief 
 * @author   bwar
 * @date:    Apr 4, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_MODULEMODEL_HPP_
#define SRC_ACTOR_CMD_MODULEMODEL_HPP_

#include <actor/Actor.hpp>

namespace neb
{

class ModuleModel: public Actor
{
public:
    ModuleModel(const std::string& strModulePath)
        : Actor(Actor::ACT_MODULE, gc_dNoTimeout), m_strModulePath(strModulePath)
    {
    }
    ModuleModel(const ModuleModel&) = delete;
    ModuleModel& operator=(const ModuleModel&) = delete;
    virtual ~ModuleModel(){};

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
                    std::shared_ptr<SocketChannel> pChannel,
                    const HttpMsg& oHttpMsg) = 0;

protected:
    const std::string& GetModulePath() const
    {
        return(m_strModulePath);
    }

private:
    std::string m_strModulePath;
    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_MODULEMODEL_HPP_ */
