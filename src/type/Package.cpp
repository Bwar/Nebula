/*******************************************************************************
 * Project:  Nebula
 * @file     Package.cpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-25
 * @note     
 * Modify history:
 ******************************************************************************/
#include "Package.hpp"

namespace neb
{

Package::Package()
{
}

Package::Package(Package&& oPackage)
{
    m_iType = oPackage.m_iType;
    m_pPayload = oPackage.m_pPayload;
    oPackage.m_iType = 0;
    oPackage.m_pPayload = nullptr;
}

Package::~Package()
{
}

Package& Package::operator=(Package&& oPackage)
{
    m_iType = oPackage.m_iType;
    m_pPayload = oPackage.m_pPayload;
    oPackage.m_iType = 0;
    oPackage.m_pPayload = nullptr;
    return(*this);
}

/*
// example
struct ChannelOption
{
    bool bPipeline = false;
    bool bWithSsl = false;
    std::string strAuth = "redis";
    std::string strPassword;
    static int Type()
    {
        return(1);
    }
};

int main(int argc, char const *argv[])
{
    ChannelOption* pOption = new ChannelOption();
    Package oPackage;
    std::cout << pOption->strAuth << std::endl;
    std::cout << "pOption = " << pOption << ", &pOption = " << &pOption << std::endl;
    oPackage.Pack(&pOption);
    std::cout << "pOption = " << pOption << ", &pOption = " << &pOption << std::endl;

    ChannelOption* pMigrateOption = nullptr;
    std::cout << "pMigrateOption = " << pMigrateOption << ", &pMigrateOption = " << &pMigrateOption << std::endl;
    std::cout << "unpack result " << oPackage.Unpack(&pMigrateOption) << std::endl;
    std::cout << "pMigrateOption = " << pMigrateOption << ", &pMigrateOption = " << &pMigrateOption << std::endl;
    std::cout << pMigrateOption->strAuth << std::endl;

    return 0;
}
*/


} /* namespace neb */

