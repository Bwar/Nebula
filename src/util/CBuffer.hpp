/*******************************************************************************
 * Project:  neb
 * @file     CBuffer.hpp
 * @brief 
 * @author   
 * @date:    2014骞�8鏈�21鏃�
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef CBUFFER_HPP_
#define CBUFFER_HPP_

#include <stdlib.h>
#include <stdarg.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/unistd.h>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

namespace neb
{

typedef unsigned int uint32_t;

/**
 *
 *       +-------------------+------------------+------------------+
 *       | readed bytes      |  readable bytes  |  writable bytes  |
 *       +-------------------+------------------+------------------+
 *       |                   |                  |                  |
 *       0      <=      readerIndex   <=   writerIndex    <=    capacity
 *
 */
class CBuffer
{
    private:
        char *m_buffer; //raw buffer
        /** total allocation available in the buffer field. */
        size_t m_buffer_len; //raw buffer length

        size_t m_write_idx;
        size_t m_read_idx;
    public:
        static const size_t BUFFER_MAX_READ = 8192;
        static const size_t DEFAULT_BUFFER_SIZE = 32;
        inline CBuffer() :
            m_buffer(NULL), m_buffer_len(0), m_write_idx(0),
                    m_read_idx(0)
        {
        }
        inline size_t GetReadIndex()
        {
            return m_read_idx;
        }
        inline size_t GetWriteIndex()
        {
            return m_write_idx;
        }
        inline void SetReadIndex(size_t idx)
        {
            m_read_idx = idx;
        }
        inline void AdvanceReadIndex(int step)
        {
            m_read_idx += step;
        }
        inline void SetWriteIndex(size_t idx)
        {
            m_write_idx = idx;
        }
        inline void AdvanceWriteIndex(int step)
        {
            m_write_idx += step;
        }
        inline bool Readable()
        {
            return m_write_idx > m_read_idx;
        }
        inline bool Writeable()
        {
            return m_buffer_len > m_write_idx;
        }
        inline size_t ReadableBytes()
        {
            return Readable() ? m_write_idx - m_read_idx : 0;
        }
        inline size_t WriteableBytes()
        {
            return Writeable() ? m_buffer_len - m_write_idx : 0;
        }

        inline size_t Compact(size_t leastLength)
        {
            uint32_t writableBytes = WriteableBytes();
            if (writableBytes < leastLength)
            {
                return 0;
            }
            uint32_t readableBytes = ReadableBytes();
            uint32_t total = Capacity();
            char* newSpace = NULL;
            if (readableBytes > 0)
            {
                newSpace = (char*) malloc(readableBytes);
                if (NULL == newSpace)
                {
                    return 0;
                }
                memcpy(newSpace, m_buffer + m_read_idx, readableBytes);
            }
            if(NULL != m_buffer)
            {
                free(m_buffer);
            }
            m_read_idx = 0;
            m_write_idx = readableBytes;
            m_buffer_len = readableBytes;
            m_buffer = newSpace;
            return total - readableBytes;
        }

        inline bool EnsureWritableBytes(size_t minWritableBytes)
        {
            if (WriteableBytes() >= minWritableBytes)
            {
                return true;
            }
            else
            {
                size_t newCapacity = Capacity();
                if (newCapacity > BUFFER_MAX_READ)
                {
                    Compact(minWritableBytes);
                }
                if (0 == newCapacity)
                {
                    newCapacity = DEFAULT_BUFFER_SIZE;
                }
                size_t minNewCapacity = GetWriteIndex()
                        + minWritableBytes;
                while (newCapacity < minNewCapacity)
                {
                    newCapacity <<= 1;
                }
                char* tmp = NULL;

                //tmp = (char*) realloc(m_buffer, newCapacity);
                tmp = (char*)malloc(newCapacity);
                if (NULL != tmp)
                {
                    memcpy(tmp, m_buffer + m_read_idx, ReadableBytes());
//                    memcpy(tmp, m_buffer, m_buffer_len);
                    free(m_buffer);
                    m_buffer = tmp;
                    m_buffer_len = newCapacity;
                    m_write_idx = ReadableBytes();
                    m_read_idx = 0;
                    return true;
                }
                return false;
            }
        }

        inline bool Reserve(size_t len)
        {
            return EnsureWritableBytes(len);
        }
        inline const char* GetRawBuffer() const
        {
            return m_buffer;
        }
        inline char* GetRawWriteBuffer() 
        {
            return m_buffer + m_write_idx;
        }
        inline const char* GetRawReadBuffer() const
        {
            return m_buffer + m_read_idx;
        }
        inline size_t Capacity() const
        {
            return m_buffer_len;
        }
        inline void Limit()
        {
            m_buffer_len = m_write_idx;
        }
        inline void Clear()
        {
            m_write_idx = m_read_idx = 0;
        }
        inline int Read(void *data_out, size_t datlen)
        {
            if (datlen > ReadableBytes())
            {
                return -1;
            }
            memcpy(data_out, m_buffer + m_read_idx, datlen);
            m_read_idx += datlen;
            return datlen;
        }
        inline int Write(const void *data_in, size_t datlen)
        {
            if (!EnsureWritableBytes(datlen))
            {
                return -1;
            }
            memcpy(m_buffer + m_write_idx, data_in, datlen);
            m_write_idx += datlen;
            return datlen;
        }

        inline int Write(CBuffer* unit, size_t datlen)
        {
            if (NULL == unit)
            {
                return -1;
            }
            if (datlen > unit->ReadableBytes())
            {
                datlen = unit->ReadableBytes();
            }
            int ret = Write(unit->m_buffer + unit->m_read_idx, datlen);
            if (ret > 0)
            {
                unit->m_read_idx += ret;
            }
            return ret;
        }

        inline int WriteByte(char ch)
        {
            return Write(&ch, 1);
        }

        inline int Read(CBuffer* unit, size_t datlen)
        {
            if (NULL == unit)
            {
                return -1;
            }
            return unit->Write(this, datlen);
        }

        inline int SetBytes(void* data, size_t datlen, size_t index)
        {
            if (NULL == data)
            {
                return -1;
            }
            if (index + datlen > m_write_idx)
            {
                return -1;
            }
            memcpy(m_buffer + index, data, datlen);
            return datlen;
        }

        inline bool ReadByte(char& ch)
        {
            return Read(&ch, 1) == 1;
        }

        inline int Copyout(void *data_out, size_t datlen)
        {
            if (datlen == 0)
            {
                return 0;
            }
            if (datlen > ReadableBytes())
            {
                datlen = ReadableBytes();
            }
            memcpy(data_out, m_buffer + m_read_idx, datlen);
            return datlen;
        }

        inline int Copyout(CBuffer* unit, size_t datlen)
        {
            if (NULL == unit || !unit->EnsureWritableBytes(datlen))
            {
                return -1;
            }
            int ret = Copyout(unit->m_buffer + +unit->m_write_idx,
                    datlen);
            if (ret > 0)
            {
                unit->m_write_idx += ret;
            }
            return ret;
            //return Copyout();
        }

        inline void SkipBytes(size_t len)
        {
            AdvanceReadIndex(len);
        }

        inline void DiscardReadedBytes()
        {
            if (m_read_idx > 0)
            {
                if (Readable())
                {
                    size_t tmp = ReadableBytes();
                    memmove(m_buffer, m_buffer + m_read_idx, tmp);
                    m_read_idx = 0;
                    m_write_idx = tmp;
                }
                else
                {
                    m_read_idx = m_write_idx = 0;
                }
            }
        }
        int IndexOf(const void* data, size_t len, size_t start,
                size_t end);
        int IndexOf(const void* data, size_t len);
        int Printf(const char *fmt, ...);
        int VPrintf(const char *fmt, va_list ap);
        int ReadFD(int fd, int& err);
        int WriteFD(int fd, int& err);
        inline CBuffer(size_t size) :
            m_buffer(NULL), m_buffer_len(0), m_write_idx(0),
                    m_read_idx(0)
        {
            EnsureWritableBytes(size);
        }

        inline std::string ToString()
        {
            return std::string(m_buffer + m_read_idx, ReadableBytes());
        }
        inline ~CBuffer()
        {
            if (m_buffer != NULL)
            {
                free(m_buffer);
            }
            m_buffer = NULL;
        }

};

}

#endif /* CBUFFER_HPP_ */
