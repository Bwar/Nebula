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
            char* szDemangleName = NULL;
            std::string strTypeName;
#ifdef __GNUC__
            szDemangleName = abi::__cxa_demangle(typeid(T).name(), NULL, NULL, NULL);
#else
            //in this format?:     szDemangleName =  typeid(T).name();
            szDemangleName = abi::__cxa_demangle(typeid(T).name(), NULL, NULL, NULL);
#endif
            if (NULL != szDemangleName)
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
        s_oRegister.do_nothing();
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
    static Register s_oRegister;
};

template<typename T, typename ...Targs>
typename DynamicCreator<T, Targs...>::Register DynamicCreator<T, Targs...>::s_oRegister;

} /* namespace neb */

#endif /* SRC_ACTOR_DYNAMICCREATOR_HPP_ */
