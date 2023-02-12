/*******************************************************************************
 * Project:  Nebula
 * @file     CodecDeliver.hpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-25
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_VIRTUAL_CODECDELIVER_HPP_
#define SRC_CODEC_VIRTUAL_CODECDELIVER_HPP_

#include <memory>
#include "channel/SpecChannel.hpp"
#include "labor/LaborShared.hpp"
#include "type/Package.hpp"

namespace neb
{

class CodecDeliver
{
public:
    CodecDeliver();
    virtual ~CodecDeliver();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_RESP);
    }

    // request
    static int Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, Package&& oPackage);

    // response
    static int Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, Package&& oPackage);
};

} /* namespace neb */

#endif /* SRC_CODEC_VIRTUAL_CODECDELIVER_HPP_ */

