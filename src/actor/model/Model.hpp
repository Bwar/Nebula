/*******************************************************************************
 * Project:  Nebula
 * @file     Model.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年6月7日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_MODEL_HPP_
#define SRC_ACTOR_MODEL_HPP_

#include "actor/Actor.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class ActorBuilder;

class Model: public Actor
{
public:
    Model()
        : Actor(Actor::ACT_MODEL, gc_dNoTimeout)
    {
    }
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    virtual ~Model(){}

    /**
     * @brief 初始化Model；重新加载配置
     * @note Model类实例初始化函数，大部分Model不需要初始化，需要初始化的Model可派生后实现此函数，
     * 在此函数里可以读取配置文件（配置文件必须为json格式）。配置文件由Model的设计者自行定义，
     * 存放于conf/目录，配置文件名最好与Model名字保持一致，加上.json后缀。配置文件的更新同步
     * 会由框架自动完成。
     *     Init()需设计成可重入方法，因各服务框架在收到Beacon的重新加载配置指令后会执行每个
     * Model的Init()方法，所以必须保证Init()方法执行后没有副作用。
     * @return 是否初始化成功
     */
    virtual bool Init()
    {
        return(true);
    }

    /**
     * @brief 提交
     */
    virtual E_CMD_STATUS Submit() = 0;

private:
    friend class ActorBuilder;
};

} /* namespace neb */

#endif /* SRC_ACTOR_MODEL_HPP_ */
