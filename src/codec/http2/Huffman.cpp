/*******************************************************************************
 * Project:  Nebula
 * @file     Huffman.cpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-04
 * @note     
 * Modify history:
 ******************************************************************************/
#include <codec/http2/Huffman.hpp>

namespace neb
{

Huffman::Huffman()
    : m_pRoot(nullptr)
{
    m_pRoot = new Node();
    BuildTree();
}

Huffman::~Huffman()
{
    if (m_pRoot != nullptr)
    {
        delete m_pRoot;
        m_pRoot = nullptr;
    }
}

Huffman* Huffman::Instance()
{
    if (m_pInstance == nullptr)
    {
        m_pInstance = new Huffman();
    }
    return(m_pInstance);
}

void Huffman::Encode(const std::string& strData, CBuffer* pBuff)
{
    uint32_t uiCurrent = 0;
    uint32_t uiNotWrittenBits = 0;
    int iByte = 0;
    uint32_t uiCode = 0;
    uint8_t ucBits = 0;
    uint8_t ucMask = 0xFF;

    for (int i = 0; i < strData.size(); ++i)
    {
        iByte = strData[i];
        uiCode = CODES[iByte];
        ucBits = CODE_LENGTHS[iByte];

        uiCurrent <<= ucBits;
        uiCurrent |= uiCode;
        uiNotWrittenBits += ucBits;

        while (uiNotWrittenBits >= 8)
        {
            uiNotWrittenBits -= 8;
            pBuff->WriteByte((uiCurrent >> uiNotWrittenBits));
        }
    }

    if (uiNotWrittenBits > 0)
    {
        uiCurrent <<= (8 - uiNotWrittenBits);
        uiCurrent |= (ucMask >> uiNotWrittenBits);
        pBuff->WriteByte((uiCurrent));
    }
}

bool Huffman::Decode(CBuffer* pBuff, uint32_t iBuffLength, std::string& strData)
{
    Node *pNode = m_pRoot;
    uint32_t uiCurrent = 0;
    uint8_t ucBits = 0;
    char c;
    for (int i = 0; i < iBuffLength; ++i)
    {
        pBuff->ReadByte(c);
        uiCurrent = (uiCurrent << 8) | c;
        ucBits += 8;
        while (ucBits >= 8)
        {
            uint32_t uiChild = (uiCurrent >> (ucBits - 8)) & 0xFF;
            pNode = pNode->vecChildren[uiChild];
            if (pNode->vecChildren.size() == 0)
            {
                // terminal pNode
                strData.append(1, pNode->ucSymbol);
                ucBits -= pNode->ucTerminalBits;
                pNode = m_pRoot;
            }
            else
            {
                // non-terminal pNode
                ucBits -= 8;
            }
        }
    }

    /* TODO confirm
       A padding strictly longer than 7 bits MUST be treated as a decoding error.
       A padding not corresponding to the most significant bits of the code for the EOS symbol MUST be treated as a decoding error.
       A Huffman-encoded string literal containing the EOS symbol MUST be treated as a decoding error.
    */
    while (ucBits > 0)
    {
        uint32_t uiChild = (uiCurrent << (8 - ucBits)) & 0xFF;
        pNode = pNode->vecChildren[uiChild];
        if (pNode->vecChildren.size() > 0 || pNode->ucTerminalBits > ucBits)
        {
            return(false);
        }
        strData.append(1, pNode->ucSymbol);
        ucBits -= pNode->ucTerminalBits;
        pNode = m_pRoot;
    }
    return(true);
}

void Huffman::BuildTree()
{
    uint8_t ucSym;
    uint32_t uiCode;
    uint8_t ucLen;
    for (int i = 0; i < 256; i++)
    {
        Node* pTerminal = new Node(ucSym, ucLen);

        Node* pCurrent = m_pRoot;
        while (ucLen > 8)
        {
            ucLen -= 8;
            int i = ((uiCode >> ucLen) & 0xFF);
            if (pCurrent->vecChildren.size() > 0)
            {
                if (pCurrent->vecChildren[i] == nullptr)
                {
                    pCurrent->vecChildren[i] = new Node();
                }
                pCurrent = pCurrent->vecChildren[i];
            }
            else    // Impossible to reach
            {
                ; //error
            }
        }

        uint8_t ucShift = 8 - ucLen;
        uint32_t uiStart = (uiCode << ucShift) & 0xFF;
        uint32_t uiEnd = 1 << ucShift;
        for (uint32_t i = uiStart; i < uiStart + uiEnd; i++)
        {
            pCurrent->vecChildren[i] = pTerminal;
        }
    }
}

} /* namespace neb */

