/*******************************************************************************
 * Project:  Nebula
 * @file     Package.hpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-25
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_TYPE_PACKAGE_HPP_
#define SRC_TYPE_PACKAGE_HPP_

#include <memory>

namespace neb
{

struct Item
{
    static int Type()
    {
        return(0);
    }

    // data
};

class Package
{
public:
    Package();
    Package(const Package&) = delete;
    Package(Package&& oPackage);
    virtual ~Package();

    Package& operator=(const Package& oPackage) = delete;
    Package& operator=(Package&& oPackage);

    int GetPayloadType() const
    {
        return(m_iType);
    }

    /**
     * @brief put item into package
     * @note put item into package, and clear source address of the item.
     * The item class must implement a static Type() method.
     */
    template<typename T>
    void Pack(std::shared_ptr<T>* item);

    template<typename T>
    void Pack(int type, std::shared_ptr<T>* item);

    /**
     * @brief take out item from package
     * @note take out item from package, and clear package.
     * The item class must implement a static Type() method.
     */
    template<typename T>
    bool Unpack(std::shared_ptr<T>* item);

    template<typename T>
    bool Unpack(int type, std::shared_ptr<T>* item);

private:
    int m_iType = 0;
    std::shared_ptr<void> m_pPayload = nullptr;
};

template<typename T>
void Package::Pack(std::shared_ptr<T>* item)
{
    m_iType = T::Type();
    m_pPayload = *item;
    *item = nullptr;
}

template<typename T>
void Package::Pack(int type, std::shared_ptr<T>* item)
{
    m_iType = type;
    m_pPayload = *item;
    *item = nullptr;
}

template<typename T>
bool Package::Unpack(std::shared_ptr<T>* item)
{
    if (T::Type() == m_iType)
    {
        *item = std::static_pointer_cast<T>(m_pPayload);
        if (*item == nullptr)
        {
            return(false);
        }
        m_pPayload = nullptr;
        m_iType = 0;
        return(true);
    }
    return(false);
}

template<typename T>
bool Package::Unpack(int type, std::shared_ptr<T>* item)
{
    if (type == m_iType)
    {
        *item = std::static_pointer_cast<T>(m_pPayload);
        if (*item == nullptr)
        {
            return(false);
        }
        m_pPayload = nullptr;
        m_iType = 0;
        return(true);
    }
    return(false);
}

} /* namespace neb */

#endif /* SRC_TYPE_PACKAGE_HPP_ */

