/*******************************************************************************
 * Project:  Nebula
 * @file     Tree.hpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-03
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_HTTP2_TREE_HPP_
#define SRC_CODEC_HTTP2_TREE_HPP_

namespace neb
{

template<typename T>
struct TreeNode
{
    T* pData                            = nullptr;
    struct TreeNode<T>* pFirstChild     = nullptr;
    struct TreeNode<T>* pRightBrother   = nullptr;
    struct TreeNode<T>* pParent         = nullptr;
};

}

#endif /* SRC_CODEC_HTTP2_TREE_HPP_ */

