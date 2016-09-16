/*
 * Attribute.cpp
 *
 *  Created on: Sep 16, 2016
 *      Author: bwar
 */


#include <dlfcn.h>
#include "object/cmd/Cmd.hpp"
#include "object/cmd/Module.hpp"
#include "Attribute.hpp"


namespace neb
{

tagSo::tagSo() : pSoHandle(NULL), pCmd(NULL), iVersion(0)
{
}

tagSo::~tagSo()
{
    if (pCmd != NULL)
    {
        DELETE(pCmd);
    }
    if (pSoHandle != NULL)
    {
        dlclose(pSoHandle);
        pSoHandle = NULL;
    }
}

tagModule::tagModule() : pSoHandle(NULL), pModule(NULL), iVersion(0)
{
}

tagModule::~tagModule()
{
    if (pModule != NULL)
    {
        DELETE(pModule);
    }
    if (pSoHandle != NULL)
    {
        dlclose(pSoHandle);
        pSoHandle = NULL;
    }
}

}

