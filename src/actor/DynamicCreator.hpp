/*******************************************************************************
 * Project:  Nebula
 * @file     ActorCreator.hpp
 * @brief 
 * @author   bwar
 * @date:    Mar 11, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_DYNAMICCREATOR_HPP_
#define SRC_ACTOR_DYNAMICCREATOR_HPP_

#include <typeinfo>
#include <cxxabi.h>
#include "ActorFactory.hpp"

namespace neb
{

template<typename T, typename...Targs>
class DynamicCreator
{
public:
    struct Register
    {
        Register()
        {
            char* szDemangleName = nullptr;
            std::string strTypeName;
#ifdef __GUNC__
            szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#else
            //in this format?:     szDemangleName =  typeid(T).name();
            szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
            if (nullptr != szDemangleName)
            {
                strTypeName = szDemangleName;
                free(szDemangleName);
            }
            ActorFactory<Targs...>::Instance()->Regist(strTypeName, CreateObject);
        }
        inline void do_nothing()const { };
    };

    DynamicCreator()
    {
        m_oRegister.do_nothing();
    }
    virtual ~DynamicCreator(){};

    static T* CreateObject(Targs&&... args)
    {
        T* pT = nullptr;
        try
        {
            pT = new T(std::forward<Targs>(args)...);
        }
        catch(std::bad_alloc& e)
        {
            //std::cerr << e.what() << std::endl;     // TODO write log
            return(nullptr);
        }
        return(pT);
    }

private:
    static Register m_oRegister;
};

template<typename T, typename ...Targs>
typename DynamicCreator<T, Targs...>::Register DynamicCreator<T, Targs...>::m_oRegister;

} /* namespace neb */

#endif /* SRC_ACTOR_DYNAMICCREATOR_HPP_ */
