/*
 * Attribute.cpp
 *
 *  Created on: Sep 16, 2016
 *      Author: bwar
 */


#include <actor/cmd/Cmd.hpp>
#include <actor/cmd/Module.hpp>
#include <dlfcn.h>
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

