/*******************************************************************************
* Project:  nebula
* @file     rc5.h
* @brief    rc5 Operations, Terminology and Notation
* @author   lbh
* @date:    2015年12月15日
* @note     In summary, the cipher will be explained in terms of these
operations:
                RC5_Key_Create          - Create a key object.
                RC5_Key_Destroy         - Destroy a key object.
                RC5_Key_Set             - Bind a user key to a key object.
                RC5_CBC_Create          - Create a cipher object.
                RC5_CBC_Destroy         - Destroy a cipher object.
                RC5_CBC_Encrypt_Init    - Bind a key object to a cipher object.
                RC5_CBC_SetIV           - Set a new IV without changing the key.
                RC5_CBC_Encrypt_Update  - Process part of a message.
                RC5_CBC_Encrypt_Final   - Process the end of a message.
The following variables will be used throughout this memo with these meanings:
    W       This is the word size for RC5 measured in bits. It is half the
block size. The word sizes covered by this memo are 32 and 64.
    WW      This is the word size for RC5 measured in bytes.
    B       This is the block size for RC5 measured in bits. It is twice
the word size. When RC5 is used as a 64 bit block cipher, B is 64 and W is 32.
0 < B < 257. In the sample code, B, is used as a variable instead of a cipher
system parameter, but this usage should be obvious from context.
    BB      This is the block size for RC5 measured in bytes. BB = B / 8.
    b       This is the byte length of the secret key. 0 <= b < 256.
    K       This is the secret key which is treated as a sequence of b
bytes indexed by: K[0], ..., K[b-1].
    R       This is the number of rounds of the inner RC5 transform.
0 <= R < 256.
    T       This is the number of words in the expanded key table. It is
always 2*(R + 1). 1 < T < 513.
    S       This is the expanded key table which is treated as a sequence
of words indexed by: S[0], ..., S[T-1].
    N       This is the byte length of the plaintext message.
    P       This is the plaintext message which is treated as a sequence of
N bytes indexed by: P[0], ..., P[N-1].
    C       This is the ciphertext output which is treated as a sequence of
bytes indexed by: C[0], C[1], ...
    I       This is the initialization vector for the CBC mode which is
treated as a sequence of bytes indexed by: I[0], ..., I[BB-1].
* Modify history:
******************************************************************************/
#ifndef SRC_UTIL_ENCRYPT_RC5_H_
#define SRC_UTIL_ENCRYPT_RC5_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define RC5_FIRST_VERSION 1

/* Definitions for RC5 as a 64 bit block cipher. */
/* The "unsigned int" will be 32 bits on all but */
/* the oldest compilers, which will make it 16 bits. */
/* On a DEC Alpha "unsigned long" is 64 bits, not 32. */
#define RC5_WORD     unsigned int
#define W            (32)
#define WW           (W / 8)
#define ROT_MASK     (W - 1)
#define BB           ((2 * W) / 8) /* Bytes per block */

/* Define macros used in multiple procedures. */
/* These macros assumes ">>" is an unsigned operation, */
/* and that x and s are of type RC5_WORD. */
#define SHL(x,s)    ((RC5_WORD)((x)<<((s)&ROT_MASK)))
#define SHR(x,s,w)  ((RC5_WORD)((x)>>((w)-((s)&ROT_MASK))))
#define ROTL(x,s,w) ((RC5_WORD)(SHL((x),(s))|SHR((x),(s),(w))))
#define SHL1(x,s,w) ((RC5_WORD)((x)<<((w)-((s)&ROT_MASK))))
#define SHR1(x,s)   ((RC5_WORD)((x)>>((s)&ROT_MASK)))
#define ROTR(x,s,w) ((RC5_WORD)(SHR1((x),(s))|SHL1((x),(s),(w))))

/**
 *  Two constants, Pw and Qw, are defined for any word size W by the expressions:
 *       Pw=Odd((e-2)*2**W)
 *       Qw=Odd((phi-1)*2**W)
 *  ｅ是自然对数的底（2.71828..．），phi是黄金比例（1.61803．．．），２**W是２的W
 *  此方，Odd（x）等于x如果x是奇数或等于x+1如果x是偶数。W等于16，32和64，Pw和
 *  where e is the base of the natural logarithm (2.71828 ...), and phi
 *  is the golden ratio (1.61803 ...), and 2**W is 2 raised to the power
 *  of W, and Odd(x) is equal to x if x is odd, or equal to x plus one if
 *  x is even. For W equal to 16, 32, and 64, the Pw and Qw constants
 *  are the following hexadecimal values:
 */
#define P16  0xb7e1
#define Q16  0x9e37
#define P32  0xb7e15163
#define Q32  0x9e3779b9
#define P64  0xb7e151628aed2a6b
#define Q64  0x9e3779b97f4a7c15
#if W == 16
#define Pw   P16 /* Select 16 bit word size */
#define Qw   Q16
#endif
#if W == 32
#define Pw   P32 /* Select 32 bit word size */
#define Qw   Q32
#endif
#if W == 64
#define Pw   P64 /* Select 64 bit word size */
#define Qw   Q64
#endif


/**
 * @brief Definition of RC5 user key object.
 * @note Like most block ciphers, RC5 expands a small user key into a table of
 * internal keys. The byte length of the user key is one of the parameters of the
 * cipher, so the RC5 user key object must be able to hold variable length keys.
 * The basic operations on a key are to create, destroy and set. To avoid exposing
 * key material to other parts of an application, the destroy operation zeros the
 * memory allocated for the key before releasing it to the memory manager. A general
 * key object may support other operations such as generating a new random key and
 * deriving a key from key-agreement information.
 */
typedef struct rc5UserKey
{
    int keyLength; /* In Bytes. */
    unsigned char *keyBytes;
} rc5UserKey;

/**
 * @brief Definition of the RC5 CBC algorithm object.
 * @note The cipher object needs to keep track of the padding mode, the number
 * of rounds, the expanded key, the initialization vector, the CBC
 * chaining block, and an input buffer.
 */
typedef struct rc5CBCAlg
{
    int Pad; /* 1 = RC5-CBC-Pad, 0 = RC5-CBC. */
    int R; /* Number of rounds. */
    RC5_WORD *S; /* Expanded key. */
    unsigned char I[BB]; /* Initialization vector. */
    unsigned char chainBlock[BB];
    unsigned char inputBlock[BB];
    int inputBlockIndex; /* Next inputBlock byte. */
} rc5CBCAlg;


/**
 * @brief Allocate and initialize an RC5 user key.
 * @note To create a key, the memory for the key object must be allocated and
 * initialized. The C code below assumes that a function called "malloc" will
 * return a block of uninitialized memory from the heap, or zero indicating an error.
 * @return Return 0 if problems.
 */
rc5UserKey *RC5_Key_Create();

/**
 * @brief Zero and free an RC5 user key.
 * @note To destroy a key, the memory must be zeroed and released to the memory
 * manager. The C code below assumes that a function called "free" will return a
 * block of memory to the heap.
 * @param pKey user key
 */
void RC5_Key_Destroy(rc5UserKey *pKey);

/**
 * @brief Set the value of an RC5 user key.
 * @note Setting the key object makes a copy of the secret key into a block of
 * memory allocated from the heap. Copy the key bytes so the caller can zero and
 * free the original.
 * @param pKey       user key
 * @param keyLength  user key length
 * @param keyBytes   user key bytes
 * @return Return zero if problems
 */
int RC5_Key_Set(rc5UserKey* pKey, int keyLength, unsigned char* keyBytes);

/**
 * @brief Expand an RC5 user key.
 * @note The key expansion routine converts the b-byte secret key, K, into an
 * expanded key, S, which is a sequence of T = 2*(R+1) words. The expansion
 * algorithm uses two constants that are derived from the constants, e, and phi.
 * These are used to initialize S, which is then modified using K.
 * @param b  Byte length of secret key
 * @param K  Secret key
 * @param R  Number of rounds
 * @param S  Expanded key buffer, 2*(R+1) words
 */
void RC5_Key_Expand(int b, unsigned char* K, int R, RC5_WORD* S);

/**
 * @brief encryption of a single input block
 * @param S
 * @param R
 * @param in
 * @param out
 */
void RC5_Block_Encrypt(RC5_WORD* S, int R, unsigned char* in, unsigned char* out);

void RC5_Block_Decrypt(RC5_WORD *S, int R, unsigned char* in, unsigned char* out);

/**
 * @brief Allocate and initialize the RC5 CBC algorithm object.
 * @note To create a cipher algorithm object, the parameters must be checked
 * and then space allocated for the expanded key table. The expanded
 * key is initialized using the method described earlier. Finally, the
 * state variables (padding mode, number of rounds, and the input
 * buffer) are set to their initial values.
 * @param Pad     1 = RC5-CBC-Pad, 0 = RC5-CBC.
 * @param R       Number of rounds.
 * @param Version RC5 version number.
 * @param bb      Bytes per RC5 block == IV len.
 * @param I       CBC IV, bb bytes long.
 * @return Return 0 if problems.
 */
rc5CBCAlg* RC5_CBC_Create(int Pad, int R, int Version, int bb, unsigned char* I);

/**
 * @brief Zero and free an RC5 algorithm object.
 * @note Destroying the cipher object is the inverse of creating it with care
 * being take to zero memory before returning it to the memory manager.
 * @param pAlg  RC5 CBC algorithm object
 */
void RC5_CBC_Destroy(rc5CBCAlg* pAlg);

/**
 * @brief Setup a new initialization vector for a CBC operation and reset the CBC object.
 * @note This can be called after Final without needing to call Init or Create again.
 * For CBC cipher objects, the state of the algorithm depends on the
 * expanded key, the CBC chain block, and any internally buffered input.
 * Often the same key is used with many messages that each have a unique
 * initialization vector. To avoid the overhead of creating a new
 * cipher object, it makes more sense to provide an operation that
 * allows the caller to change the initialization vector for an existing
 * cipher object.
 * @param pAlg RC5 CBC algorithm object
 * @param I    CBC Initialization vector, BB bytes.
 * @return Return zero if problems.
 */
int RC5_CBC_SetIV(rc5CBCAlg* pAlg, unsigned char* I);

/**
 * @brief Binding a key to a cipher object
 * @note Initialize the encryption object with the given key. After this routine, the
 * caller frees the key object. The IV for this CBC object can be changed by calling
 * the SetIV routine. The only way to change the key is to destroy the CBC object and
 * create a new one.
 * The operation that binds a key to a cipher object performs the key expansion. Key
 * expansion could be an operation on keys, but that would not work correctly for
 * ciphers that modify the expanded key as they operate. After expanding the key,
 * this operation must initialize the CBC chain block from the initialization vector
 * and prepare the input buffer to receive the first character.
 * @param pAlg  RC5 CBC algorithm object
 * @param pKey  user key
 * @return Return zero if problems.
 */
int RC5_CBC_Encrypt_Init(rc5CBCAlg* pAlg, rc5UserKey* pKey);

/**
 * @brief Encrypt a buffer of plaintext.
 * @note The plaintext and ciphertext buffers can be the same. The byte len of the
 * ciphertext is put in *pCipherLen. Call this multiple times passing successive
 * parts of a large message. After the last part has been passed to Update, call Final.
 * The encryption process described here uses the Init-Update-Final paradigm. The
 * update operation can be performed on a sequence of message parts in order to
 * incrementally produce the ciphertext. After the last part is processed, the Final
 * operation is called to pick up any plaintext bytes or padding that are buffered
 * inside the cipher object.
 * @param pAlg          Cipher algorithm object.
 * @param N             Byte length of P.
 * @param P             Plaintext buffer.
 * @param pCipherLen    Gets byte len of C.
 * @param maxCipherLen  Size of C.
 * @param C             Ciphertext buffer.
 * @return Return zero if problems like output buffer too small.
 */
int RC5_CBC_Encrypt_Update(rc5CBCAlg* pAlg, int N, unsigned char* P, int* pCipherLen, int maxCipherLen, unsigned char* C);

/**
 * @brief Produce the final block of ciphertext including any padding, and then reset
 * the algorithm object.
 * @note This step handles the last block of plaintext. For RC5-CBC, this step just
 * performs error checking to ensure that the plaintext length was indeed a multiple
 * of the block length. For RC5-CBC-Pad, padding bytes are added to the plaintext.
 * The pad bytes are all the same and are set to a byte that represents the number
 * of bytes of padding. For example if there are eight bytes of padding, the bytes
 * will all have the hexadecimal value 0x08. There will be between one and BB padding
 * bytes, inclusive.
 * @param pAlg          Cipher algorithm object.
 * @param pCipherLen    Gets byte len of C.
 * @param maxCipherLen  Len of C buffer.
 * @param C             Ciphertext buffer.
 * @return Return zero if problems.
 */
int RC5_CBC_Encrypt_Final(rc5CBCAlg* pAlg, int* pCipherLen, int maxCipherLen, unsigned char* C);

int RC5_CBC_Decrypt_Init(rc5CBCAlg *pAlg, rc5UserKey *pKey);

int RC5_CBC_Decrypt_Update(rc5CBCAlg *pAlg, int N, unsigned char* C, int *plainLen, unsigned char* P);

#endif /* SRC_UTIL_ENCRYPT_RC5_H_ */
