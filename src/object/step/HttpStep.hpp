/*******************************************************************************
 * Project:  Nebula
 * @file     HttpStep.hpp
 * @brief    Http服务的异步步骤基类
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_STEP_HTTPSTEP_HPP_
#define SRC_OBJECT_STEP_HTTPSTEP_HPP_

#include "util/http/http_parser.h"
#include "Step.hpp"

namespace neb
{

class HttpStep: public Step
{
public:
    HttpStep(Step* pNextStep = NULL);
    HttpStep(const tagChannelContext& stCtx, Step* pNextStep = NULL);
    virtual ~HttpStep();

    virtual E_CMD_STATUS Callback(
                    const tagChannelContext& stCtx,
                    const HttpMsg& oHttpMsg,
                    void* data = NULL) = 0;

    bool HttpPost(const std::string& strUrl, const std::string& strBody, const std::map<std::string, std::string>& mapHeaders);
    bool HttpGet(const std::string& strUrl);

protected:
    bool HttpRequest(const HttpMsg& oHttpMsg);

    tagChannelContext m_stCtx;
};

} /* namespace neb */

#endif /* SRC_OBJECT_STEP_HTTPSTEP_HPP_ */
