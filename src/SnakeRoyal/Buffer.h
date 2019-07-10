#pragma once

#ifndef _BUFFER_H_
#define _BUFFER_H_
#pragma once

#include <stdint.h>
#include <memory>
#include <assert.h>

enum class BufferSeek
{
    SET,
    END,
    CUR,
};

template<typename T> class BufferBase
{
private:
    T* m_buffer;
    size_t m_capacity;
    size_t m_size;
    size_t m_offset;
    bool m_borrowed;

public:
    BufferBase();
    ~BufferBase();

    BufferBase(const BufferBase& other)
    {
        *this = other;
    }

    BufferBase& operator=(const BufferBase& other)
    {
        purge();
        write(other.base(), other.size());
        return *this;
    }

    // Move semantics
    BufferBase(BufferBase&& other)
    {
        m_buffer = other.m_buffer;
        m_size = other.m_size;
        m_offset = other.m_offset;
        m_capacity = other.m_capacity;
        m_borrowed = other.m_borrowed;

        // Release from the other object, we can't delete it.
        other.m_buffer = nullptr;
        other.m_size = 0;
    }

    BufferBase& operator=(BufferBase&& other)
    {
        if (this != &other)
        {
            purge();

            m_buffer = other.m_buffer;
            m_size = other.m_size;
            m_offset = other.m_offset;
            m_capacity = other.m_capacity;

            // Release from the other object, we can't delete it.
            other.m_buffer = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

    // Write data at the current position, overwrites,
    size_t write(const void* data, size_t size);

    size_t write_str(const char* str)
    {
        return write((const void*)str, (strlen(str) + 1) * sizeof(char));
    }

    size_t write_str(const wchar_t* str)
    {
        return write((const void*)str, (wcslen(str) + 1) * sizeof(wchar_t));
    }

    template<typename D> size_t write(const D& data)
    {
        return write(&data, sizeof(D));
    }

    size_t write(const BufferBase<T>& data)
    {
        return write(data.base(), data.size());
    }

    // Just expands the buffer and returns the offset, this allows direct
    // writing.
    size_t room(size_t len);

    // Insert data at the current position, extends buffer.
    size_t insert(const void* data, size_t size);

    size_t insert_str(const char* str)
    {
        return insert(str, (strlen(str) + 1) * sizeof(char));
    }

    size_t insert_str(const wchar_t* str)
    {
        return insert(str, (wcslen(str) + 1) * sizeof(wchar_t));
    }

    // Read data from the current position.
    size_t read(void* data, size_t size);

    template<typename D> size_t read(D& data)
    {
        return read(&data, sizeof(D));
    }

    size_t seek(size_t offset, BufferSeek method = BufferSeek::SET);

    size_t erase(size_t size);

    size_t offset() const;

    bool eob() const;

    bool empty() const;

    size_t size() const;
    size_t capacity() const;

    void reserve(size_t newCapacity);

    T& operator[](size_t index);

    void clear();
    void purge();

    T* base() const;

private:
    T* reallocate(T* buf, size_t count)
    {
        size_t byteSize = sizeof(T) * count;

        void* res = nullptr;
        if (buf)
            res = ::realloc(buf, byteSize);
        else
            res = ::malloc(byteSize);

        return static_cast<T*>(res);
    }

    void dealloc(T* buf)
    {
        if (!buf || m_borrowed == true)
            return;

        ::free(buf);
    }

private:
    void grow(size_t size, bool insertType);
};

template<typename T> bool BufferBase<T>::empty() const
{
    return m_size == 0;
}

template<typename T> T& BufferBase<T>::operator[](size_t index)
{
    return m_buffer[index];
}

template<typename T> BufferBase<T>::BufferBase()
{
    m_buffer = nullptr;
    m_capacity = 0;
    m_offset = 0;
    m_size = 0;
    m_borrowed = false;
}

template<typename T> BufferBase<T>::~BufferBase()
{
    purge();
}

template<typename T> void BufferBase<T>::grow(size_t size, bool insertType)
{
    size_t spaceLeft = (m_size - m_offset);

    if (m_borrowed)
    {
        if (size > spaceLeft)
        {
            assert(false);
        }
        return;
    }

    if (m_offset + size > m_size)
    {
        if (m_offset + size < m_capacity)
        {
            // Capacity space left.
            if (spaceLeft < size)
                size -= spaceLeft;

            m_size += size;
            return;
        }
    }
    else
    {
        // Space left, however insert always expands size.
        if (insertType)
        {
            m_size += size;
            return;
        }
        return;
    }

    size_t newCapcity = (m_capacity + size + 1) << 1;

    m_buffer = reallocate(m_buffer, newCapcity);

    if (insertType)
        m_size += size;
    else
    {
        m_size += (size - spaceLeft);
    }

    m_capacity = newCapcity;
}

template<typename T> size_t BufferBase<T>::write(const void* data, size_t size)
{
    grow(size, false);

    memcpy(m_buffer + m_offset, data, size);
    m_buffer[m_size] = '\0';

    m_offset += size;

    return size;
}

template<typename T> size_t BufferBase<T>::room(size_t len)
{
    grow(len, false);

    size_t offset = m_offset;
    m_offset += len;

    return offset;
}

template<typename T> size_t BufferBase<T>::insert(const void* data, size_t size)
{
    grow(size, true);

    size_t sizeOfEnd = m_size - (m_offset + size);

    memmove(m_buffer + m_offset + size, m_buffer + m_offset, sizeOfEnd);
    memcpy(m_buffer + m_offset, data, size);
    m_buffer[m_size] = '\0';

    m_offset += size;

    return size;
}

template<typename T> size_t BufferBase<T>::erase(size_t size)
{
    if (empty())
        return 0;

    if (m_offset + size > m_size)
        size = (m_size - m_offset);

    size_t sizeOfEnd = m_size - (m_offset + size);

    memmove(m_buffer + m_offset, m_buffer + m_offset + size, sizeOfEnd);
    m_size -= size;
    m_buffer[m_size] = '\0';

    return size;
}

template<typename T> size_t BufferBase<T>::read(void* data, size_t size)
{
    if (m_offset + size > m_size)
        size = (m_size - m_offset);

    memcpy(data, m_buffer + m_offset, size);
    m_offset += size;

    return size;
}

template<typename T> bool BufferBase<T>::eob() const
{
    return m_offset >= m_size;
}

template<typename T> size_t BufferBase<T>::size() const
{
    return m_size;
}

template<typename T> size_t BufferBase<T>::capacity() const
{
    return m_capacity;
}

template<typename T>
size_t BufferBase<T>::seek(
    size_t offset, BufferSeek method /*= BufferSeek::SET*/)
{
    switch (method)
    {
        case BufferSeek::SET:
            m_offset = 0;
        case BufferSeek::CUR:
        {
            size_t maxOffset = offset;

            if (maxOffset + m_offset > m_size)
                maxOffset = m_size - m_offset;

            m_offset += maxOffset;
        }
        break;
        case BufferSeek::END:
        {
            if (m_size - offset > m_size)
                m_offset = 0;
            else
                m_offset = (m_size - offset);
        }
        break;
    }
    return m_offset;
}

template<typename T> size_t BufferBase<T>::offset() const
{
    return m_offset;
}

template<typename T> void BufferBase<T>::reserve(size_t newCapacity)
{
    if (newCapacity > m_capacity)
    {
        T* buffer = new T[newCapacity];
        if (m_buffer)
        {
            memcpy(buffer, m_buffer, m_size);
        }
        m_buffer = buffer;
        m_capacity = newCapacity;
    }
}

template<typename T> void BufferBase<T>::clear()
{
    // Capacity and buffer remain set.
    m_size = 0;
    m_offset = 0;
    if (m_buffer)
        m_buffer[0] = '\0';
}

template<typename T> void BufferBase<T>::purge()
{
    dealloc(m_buffer);
    m_size = 0;
    m_capacity = 0;
    m_buffer = nullptr;
    m_offset = 0;
}

template<typename T> T* BufferBase<T>::base() const
{
    return m_buffer;
}

typedef BufferBase<uint8_t> Buffer;

#endif // _BUFFER_H_