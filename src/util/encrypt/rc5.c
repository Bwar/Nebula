/*******************************************************************************
 * Project:  nebula
 * @file     rc5.cpp
 * @brief
 * @author   lbh
 * @date:    2015年12月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "rc5.h"

rc5UserKey *RC5_Key_Create()
{
    rc5UserKey *pKey;

    pKey = (rc5UserKey *) malloc(sizeof(*pKey));
    if (pKey != ((rc5UserKey *) 0))
    {
        pKey->keyLength = 0;
        pKey->keyBytes = (unsigned char *) 0;
    }
    return (pKey);
}

void RC5_Key_Destroy(rc5UserKey* pKey)
{
    unsigned char *to;
    int count;

    if (pKey == ((rc5UserKey *) 0))
        return;
    if (pKey->keyBytes == ((unsigned char *) 0))
        return;
    to = pKey->keyBytes;
    for (count = 0; count < pKey->keyLength; count++)
        *to++ = (unsigned char) 0;
    free(pKey->keyBytes);
    pKey->keyBytes = (unsigned char *) 0;
    pKey->keyLength = 0;
    free(pKey);
}

int RC5_Key_Set(rc5UserKey * pKey, int keyLength, unsigned char* keyBytes)
{
    unsigned char *keyBytesCopy;
    unsigned char *from, *to;
    int count;

    keyBytesCopy = (unsigned char *) malloc(keyLength);
    if (keyBytesCopy == ((unsigned char *) 0))
        return (0);
    from = keyBytes;
    to = keyBytesCopy;
    for (count = 0; count < keyLength; count++)
        *to++ = *from++;
    pKey->keyLength = count;
    pKey->keyBytes = keyBytesCopy;
    return (1);
}

void RC5_Key_Expand(int b, unsigned char* K, int R, RC5_WORD* S)
{
    /*
     * 这个步骤转换了b-byte密钥为一个存储在L数组中的字序列。在一个小端字节序的
     * 处理器上通过将L数组置零再拷贝K的b个字节实现。下面的代码将在所有的处理器
     * 上获得这个效果：
     */
    int i, j, k, LL, t, T;
    RC5_WORD L[256 / WW]; /* Based on max key size */
    RC5_WORD A, B;

    /* LL is number of elements used in L. */
    LL = (b + WW - 1) / WW;
    for (i = 0; i < LL; i++)
    {
        L[i] = 0;
    }
    for (i = 0; i < b; i++)
    {
        t = (K[i] & 0xFF) << (8 * (i % 4)); /* 0, 8, 16, 24*/
        L[i / WW] = L[i / WW] + t;
    }

    /*
     * 这一步使用一个基于Pw加Qw再模２的W次方的算术级的固定伪随机数模式来填充S表。
     * 元素S[i]等于i*Qw+Pw模２的W次方。这个表可以被预计算和按所需进行拷贝或在空
     * 闲时间计算。
     */
    T = 2 * (R + 1);
    S[0] = Pw;
    for (i = 1; i < T; i++)
    {
        S[i] = S[i - 1] + Qw;
    }

    /*
     * 这一步混合密钥Ｋ到扩展密钥Ｓ。混合函数的循环次数Ｋ为被初始化的Ｌ元素的个数
     * 的３倍，表示为LL，S中的元素个数表示为Ｔ。每个循环类似一个加密内部循环的反
     * 复因为两个变量A和B是由这个加密循环的第一部分和第二部分更新的。
     *     A和B的初始值为０，i作为Ｓ数组的索引，j作为Ｌ数组的索引。第一个循环左移
     * 式的局部结果通过计算S[i]，A和B的和得到。然后将这个结果循环左移３位后赋给A。
     * 接着将A的值赋给S[i]。第二个循环左移的局部结果通过计算Ｌ[i]，A和B的和得到。
     * 然后将这个结果循环左移A+B位后赋给B。接着将B的值赋给L[j]。在循环结束前i、j
     * 的值加１模各自对应的数组的长度。
     */
    i = j = 0;
    A = B = 0;
    if (LL > T)
        k = 3 * LL; /* Secret key len > expanded key. */
    else
        k = 3 * T; /* Secret key len < expanded key. */
    for (; k > 0; k--)
    {
        A = ROTL(S[i] + A + B, 3, W);
        S[i] = A;
        B = ROTL(L[j] + A + B, A + B, W);
        L[j] = B;
        i = (i + 1) % T;
        j = (j + 1) % LL;
    }
    return;
} /* End of RC5_Key_Expand */

/*
 * 这部分通过解释对一个简单输入块进行加密操作的步骤来说明RC5块密码。解密处理与加密
 * 处理步骤相反，因此在此不对此进行解释。RC5密码的参数有一个版本号,V,一个轮数,R,一
 * 个以位计数的字尺寸,W。此处的描述对应RC5的原始版本（V=16 十六进制数）囊括了R的任
 * 意正值和W的值16,32和64。这一处理的输入是密钥扩展表，S，轮数，R，输入缓冲区的指针，
 * in，和输出缓冲区的指针，out。
 */
void RC5_Block_Encrypt(RC5_WORD* S, int R, unsigned char* in, unsigned char* out)
{
    /*
     * 加载A和B的值
     * 这一步转换输入字节为两个无符号整数称作A和B。当RC5被用作64位块密码A和B是32位
     * 的值。第一个输入的字节作为A的低位字节。第四个输入的字节作为A的高位字字节，第
     * 五个输入字节作为B的低位字节，最后一个输入的字节作为B的高位字节。这种转换对于
     * 小端字节序处理器是非常有效率的例如Intel系列。
     */
    int i;
    RC5_WORD A, B;

    A = in[0] & 0xFF;
    A += (in[1] & 0xFF) << 8;
    A += (in[2] & 0xFF) << 16;
    A += (in[3] & 0xFF) << 24;
    B = in[4] & 0xFF;
    B += (in[5] & 0xFF) << 8;
    B += (in[6] & 0xFF) << 16;
    B += (in[7] & 0xFF) << 24;

    /*
     * 重申轮函数
     * 这一步将扩展密钥与输入混合在一起做基本的加密操作。扩展密钥数组的头两个字被分别
     * 填充到A和B，然后轮函数被重复R次。
     *     轮函数的前半部分基于A、B和扩展密钥中下一个未用的字的值为A计算一个新的值。
     * 首先A与B异或然后将此结果循环左移B次。循环每次循环W位（例如：对于有64位块密码的
     * RC5版本循环32位）。实际的循环次数至少是B的W位以2为底的对数。接着加扩展密钥数组
     * 的下一个未用的字形成A的新值。
     *     轮函数的后半部分操作是相同的除了A、B的角色互换一下。特别B保存A与B异或的结果
     * 然后将此结果循环左移A次，接着加上下一个扩展密钥数组中未用的字成为B的新值。
     */
    A = A + S[0];
    B = B + S[1];
    for (i = 1; i <= R; i++)
    {
        A = A ^ B;
        A = ROTL(A, B, W) + S[2 * i];
        B = B ^ A;
        B = ROTL(B, A, W) + S[(2 * i) + 1];
    }

    /*
     * 存储A和B的值
     * 最后一步是转换A和B为一个字节序列。这是打开操作的反变换。
     */
    out[0] = (A >> 0) & 0xFF;
    out[1] = (A >> 8) & 0xFF;
    out[2] = (A >> 16) & 0xFF;
    out[3] = (A >> 24) & 0xFF;
    out[4] = (B >> 0) & 0xFF;
    out[5] = (B >> 8) & 0xFF;
    out[6] = (B >> 16) & 0xFF;
    out[7] = (B >> 24) & 0xFF;
    return;
} /* End of RC5_Block_Encrypt */

void RC5_Block_Decrypt(RC5_WORD *S, int R, unsigned char*in, unsigned char*out)
{
    int i;
    RC5_WORD A, B;
    A = in[0] & 0xFF;
    A += (in[1] & 0xFF) << 8;
    A += (in[2] & 0xFF) << 16;
    A += (in[3] & 0xFF) << 24;
    B = in[4] & 0xFF;
    B += (in[5] & 0xFF) << 8;
    B += (in[6] & 0xFF) << 16;
    B += (in[7] & 0xFF) << 24;
    for (i = R; i >= 1; i--)
    {
        B = ROTR((B - S[2 * i + 1]), A, W);
        B = B ^ A;
        A = ROTR((A - S[2 * i]), B, W);
        A = A ^ B;
    }
    B = B - S[1];
    A = A - S[0];
    out[0] = (A >> 0) & 0xFF;
    out[1] = (A >> 8) & 0xFF;
    out[2] = (A >> 16) & 0xFF;
    out[3] = (A >> 24) & 0xFF;
    out[4] = (B >> 0) & 0xFF;
    out[5] = (B >> 8) & 0xFF;
    out[6] = (B >> 16) & 0xFF;
    out[7] = (B >> 24) & 0xFF;
    return;
}/*End of RC5_Block_Decrypt */

/*
 *  创建一个密码算法对象，参数必须被检查然后为扩展密钥表分配空间。扩展密钥使用早
 *  先描述的方法来进行初始化。最后，状态变量（填充模式、轮数和输入缓冲区）被赋予
 *  初始值。
 */
rc5CBCAlg *RC5_CBC_Create(int Pad, int R, int Version, int bb, unsigned char* I)
{
    rc5CBCAlg *pAlg;
    int index;

    if ((Version != RC5_FIRST_VERSION) || (bb != BB) || (R < 0) || (255 < R))
        return ((rc5CBCAlg *) 0);
    pAlg = (rc5CBCAlg *) malloc(sizeof(*pAlg));
    if (pAlg == ((rc5CBCAlg *) 0))
        return ((rc5CBCAlg *) 0);
    pAlg->S = (RC5_WORD *) malloc(BB * (R + 1));
    if (pAlg->S == ((RC5_WORD *) 0))
    {
        free(pAlg);
        return ((rc5CBCAlg *) 0);
    }
    pAlg->Pad = Pad;
    pAlg->R = R;
    pAlg->inputBlockIndex = 0;
    for (index = 0; index < BB; index++)
        pAlg->I[index] = I[index];
    return (pAlg);
}

/*
 * 撤消密码是创建它的反变换所关心的问题是在将存储空间返回给存储管理者之前将其值置为0。
 */
void RC5_CBC_Destroy(rc5CBCAlg* pAlg)
{
    RC5_WORD *to;
    int count;

    if (pAlg == ((rc5CBCAlg *) 0))
        return;
    if (pAlg->S == ((RC5_WORD *) 0))
        return;
    to = pAlg->S;
    for (count = 0; count < (1 + pAlg->R); count++)
    {
        *to++ = 0; /* Two expanded key words per round. */
        *to++ = 0;
    }
    free(pAlg->S);
    for (count = 0; count < BB; count++)
    {
        pAlg->I[count] = (unsigned char) 0;
        pAlg->inputBlock[count] = (unsigned char) 0;
        pAlg->chainBlock[count] = (unsigned char) 0;
    }
    pAlg->Pad = 0;
    pAlg->R = 0;
    pAlg->inputBlockIndex = 0;
    free(pAlg);
}

/*
 * 为密码对象设置初始向量
 * 对于CBC密码对象，算法的状态依赖于扩展密钥，CBC链接块和任何内部缓存的输入。经常
 * 相同的密钥被许多管理者使用每一个拥有独一无二的初始向量。消除创建新的密码对象的
 * 系统开销，提供一个允许调用者为一个以存在的密码对象改变初始向量的操作将更有意义。
 */
int RC5_CBC_SetIV(rc5CBCAlg* pAlg, unsigned char* I)
{
    int index;

    pAlg->inputBlockIndex = 0;
    for (index = 0; index < BB; index++)
    {
        pAlg->I[index] = pAlg->chainBlock[index] = I[index];
        pAlg->inputBlock[index] = (unsigned char) 0;
    }
    return (1);
}

/*
 * 绑定一个密钥到一个密码对象
 * 绑定一个密钥到一个密码对象的操作执行了密钥扩展。密钥扩展可能是在密钥上进行的一个
 * 操作，但是当他们操作时修改扩展密钥将使之不能为正确的为密码工作。扩展密钥后，这个
 * 操作必须用初始向量初始化CBC链接块并且为接受第一个字符准备缓冲区。
 */
int RC5_CBC_Encrypt_Init(rc5CBCAlg* pAlg, rc5UserKey* pKey)
{
    if ((pAlg == ((rc5CBCAlg *) 0)) || (pKey == ((rc5UserKey *) 0)))
        return (0);
    RC5_Key_Expand(pKey->keyLength, pKey->keyBytes, pAlg->R, pAlg->S);
    return (RC5_CBC_SetIV(pAlg, pAlg->I));
}

/*
 * 此处的加密操作使用Init-Update-Final过程描述。Update操作被用于消息部分的一个序列
 * 为了递增的产生密文。在最后部分被处理后，Final被调用获得任意的明文字节或填充被缓存
 * 在密码对象内的明文字节。
 */
int RC5_CBC_Encrypt_Update(rc5CBCAlg* pAlg, int N, unsigned char* P, int* pCipherLen,
                int maxCipherLen, unsigned char* C)
{
    /*
     * 输出缓冲区大小的检查
     * 明文处理的第一步是确保输出缓冲区有足够大的空间存储密文。密文将以块大小的倍数
     * 被产生依赖于被传递给这个操作的明文字符的个数加任意位于密码对象内部缓冲区的字符。
     */
    int plainIndex, cipherIndex, j;

    /* Check size of the output buffer. */
    if (maxCipherLen < (((pAlg->inputBlockIndex + N) / BB) * BB))
    {
        *pCipherLen = 0;
        return (0);
    }

    /*
     * 将明文分成块
     * 下一步是填充字符到内部的缓冲区直到一个块被填满。此时，缓冲区指针被复位输入缓冲
     * 区和CBC链接密码块相异或。链接密码块的字节序和输入块相同。例如：第9个输入字节和
     * 第一个密文字节相异或。结果然后被传递给先前描述的RC5块密码。为了减少数据的移动
     * 和字节调整的问题，RC5的输出能被直接的写到CBC链接块。最后，这个输出被拷贝到由用
     * 户提供的密文缓冲区。在返回前，密文的实际大小被传递给调用者。
     */
    plainIndex = cipherIndex = 0;
    while (plainIndex < N)
    {
        if (pAlg->inputBlockIndex < BB)
        {
            pAlg->inputBlock[pAlg->inputBlockIndex] = P[plainIndex];
            pAlg->inputBlockIndex++;
            plainIndex++;
        }
        if (pAlg->inputBlockIndex == BB)
        { /* Have a complete input block, process it. */
            pAlg->inputBlockIndex = 0;
            for (j = 0; j < BB; j++)
            { /* XOR in the chain block. */
                pAlg->inputBlock[j] = pAlg->inputBlock[j] ^ pAlg->chainBlock[j];
            }
            RC5_Block_Encrypt(pAlg->S, pAlg->R, pAlg->inputBlock,
                            pAlg->chainBlock);
            for (j = 0; j < BB; j++)
            { /* Output the ciphertext. */
                C[cipherIndex] = pAlg->chainBlock[j];
                cipherIndex++;
            }
        }
    }
    *pCipherLen = cipherIndex;
    return (1);
} /* End of RC5_CBC_Encrypt_Update */

/*
 * 最后块的处理
 * 这一步处理明文的最后一个块。对于RC5-CBC，这一步只是做错误检查确保明文长度确实是
 * 块长度的倍数。对于RC5-CBC-Pad，填充的字节被添加到明文。填充的字节都是相同的其值
 * 被置为填充的字节数。例如如果填充了8个字节，填充的字节其值都为16进制的0x08。将包
 * 含1到BB个填充字节。
 */
int RC5_CBC_Encrypt_Final(rc5CBCAlg* pAlg, int* pCipherLen, int maxCipherLen,
                unsigned char* C)
{
    int cipherIndex, j;
    int padLength;

    /* For non-pad mode error if input bytes buffered. */
    *pCipherLen = 0;
    if ((pAlg->Pad == 0) && (pAlg->inputBlockIndex != 0))
        return (0);

    if (pAlg->Pad == 0)
        return (1);
    if (maxCipherLen < BB)
        return (0);

    padLength = BB - pAlg->inputBlockIndex;
    for (j = 0; j < padLength; j++)
    {
        pAlg->inputBlock[pAlg->inputBlockIndex] = (unsigned char) padLength;
        pAlg->inputBlockIndex++;
    }
    for (j = 0; j < BB; j++)
    { /* XOR the chain block into the plaintext block. */
        pAlg->inputBlock[j] = pAlg->inputBlock[j] ^ pAlg->chainBlock[j];
    }
    RC5_Block_Encrypt(pAlg->S, pAlg->R, pAlg->inputBlock, pAlg->chainBlock);
    cipherIndex = 0;
    for (j = 0; j < BB; j++)
    { /* Output the ciphertext. */
        C[cipherIndex] = pAlg->chainBlock[j];
        cipherIndex++;
    }
    *pCipherLen = cipherIndex;

    /* Reset the CBC algorithm object. */
    return (RC5_CBC_SetIV(pAlg, pAlg->I));
} /* End of RC5_CBC_Encrypt_Final */

int RC5_CBC_Decrypt_Init(rc5CBCAlg* pAlg, rc5UserKey* pKey)
{
    if ((pAlg == ((rc5CBCAlg *) 0)) || (pKey == ((rc5UserKey *) 0)))
        return (0);
    RC5_Key_Expand(pKey->keyLength, pKey->keyBytes, pAlg->R, pAlg->S);
    return (RC5_CBC_SetIV(pAlg, pAlg->I));
}

int RC5_CBC_Decrypt_Update(rc5CBCAlg *pAlg, int N, unsigned char* C, int* plainLen, unsigned char* P)
{
    int plainIndex, cipherIndex, j;
    plainIndex = cipherIndex = 0;
    for (j = 0; j < BB; j++)
    {
        P[plainIndex] = pAlg->chainBlock[j];
        plainIndex++;
    }
    plainIndex = 0;
    while (cipherIndex < N)
    {
        if (pAlg->inputBlockIndex < BB)
        {
            pAlg->inputBlock[pAlg->inputBlockIndex] = C[cipherIndex];
            pAlg->inputBlockIndex++;
            cipherIndex++;
        }
        if (pAlg->inputBlockIndex == BB)
        {
            pAlg->inputBlockIndex = 0;
            RC5_Block_Decrypt(pAlg->S, pAlg->R, pAlg->inputBlock, pAlg->chainBlock);
            for (j = 0; j < BB; j++)
            {
                if (plainIndex < BB)
                    P[plainIndex] ^= pAlg->chainBlock[j];
                else
                    P[plainIndex] = C[cipherIndex - 16 + j]
                                    ^ pAlg->chainBlock[j];
                plainIndex++;
            }
        }
    }
    *plainLen = plainIndex;
    return (1);
}/*End of RC5_CBC_Decrypt_Update*/
