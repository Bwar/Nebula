/*******************************************************************************
 * Project:  Nebula
 * @file     Huffman.hpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-04
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_HTTP2_HUFFMAN_HPP_
#define SRC_CODEC_HTTP2_HUFFMAN_HPP_

#include <string>
#include <vector>
#include "util/CBuffer.hpp"

namespace neb
{

struct Node
{
    // Terminal nodes have a symbol.
    uint8_t ucSymbol;
    // Number of bits represented in the terminal node.
    uint8_t ucTerminalBits;
    // size() = 0 if terminal.
    std::vector<Node*> vecChildren;

    Node()
    {
        ucSymbol = 0;
        ucTerminalBits = 0;
        vecChildren.resize(256, nullptr);
    }

    Node(uint8_t ucSym, uint8_t ucBits)
    {
        ucSymbol = ucSym;
        uint8_t b = ucBits & 0x07;
        ucTerminalBits = (b == 0) ? 8 : b;
    }

    Node(const Node& stNode) = delete;

    Node(Node&& stNode)
    {
        ucSymbol = stNode.ucSymbol;
        stNode.ucSymbol = 0;
        ucTerminalBits = stNode.ucTerminalBits;
        stNode.ucTerminalBits = 0;
        vecChildren = std::move(stNode.vecChildren);
    }

    virtual ~Node()
    {
        for (auto it = vecChildren.begin(); it != vecChildren.end(); ++it)
        {
            if (*it != nullptr)
            {
                delete (*it);
            }
        }
        vecChildren.clear();
    }

    Node& operator=(const Node& stNode) = delete;

    Node& operator=(Node&& stNode)
    {
        ucSymbol = stNode.ucSymbol;
        stNode.ucSymbol = 0;
        ucTerminalBits = stNode.ucTerminalBits;
        stNode.ucTerminalBits = 0;
        vecChildren = std::move(stNode.vecChildren);
        return(*this);
    }
};

class Huffman
{
public:
    virtual ~Huffman();

    static Huffman* Instance();

    void Encode(const std::string& strData, CBuffer* pBuff);
    bool Decode(CBuffer* pBuff, int iBuffLength, std::string& strData);

protected:
    void BuildTree();

private:
    Huffman();
    Node* m_pRoot;

    static Huffman* m_pInstance;
    static const uint32_t CODES[];
    static const uint8_t CODE_LENGTHS[];
};

} /* namespace neb */

#endif /* SRC_CODEC_HTTP2_HUFFMAN_HPP_ */

