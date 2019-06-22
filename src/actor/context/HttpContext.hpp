/*******************************************************************************
 * Project:  Nebula
 * @file     HttpContext.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年2月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_HTTPCONTEXT_HPP_
#define SRC_ACTOR_HTTPCONTEXT_HPP_

#include "pb/http.pb.h"
#include "labor/Worker.hpp"
#include "actor/DynamicCreator.hpp"
#include "Context.hpp"

namespace neb
{

class HttpContext: public Context
{
public:
    HttpContext();
    HttpContext(
            std::shared_ptr<SocketChannel> pChannel);
    HttpContext(
            std::shared_ptr<SocketChannel> pChannel,
            const HttpMsg& oReqHttpMsg);
    HttpContext(const HttpContext&) = delete;
    HttpContext& operator=(const HttpContext&) = delete;
    virtual ~HttpContext();

public:
    /**
     * @brief 给请求方发响应
     * @param strData 响应结果或错误信息
     */
    bool Response(const std::string& strData);

private:
    HttpMsg m_oReqHttpMsg;
    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_HTTPCONTEXT_HPP_ */
