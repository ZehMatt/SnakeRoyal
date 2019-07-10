#pragma once

#include "Buffer.h"

#include <array>
#include <vector>

template<typename T> struct Serializer;

template<typename T> struct SerializerInteger
{
    static bool serialize(Buffer& buffer, const T& data)
    {
        return buffer.write(data);
    }
    static bool deserialize(Buffer& buffer, T& data)
    {
        return buffer.read(data);
    }
};

template<> struct Serializer<uint8_t> : SerializerInteger<uint8_t>
{
};

template<> struct Serializer<uint16_t> : SerializerInteger<uint16_t>
{
};

template<> struct Serializer<uint32_t> : SerializerInteger<uint32_t>
{
};

template<> struct Serializer<uint64_t> : SerializerInteger<uint64_t>
{
};

template<typename T> struct Serializer<std::vector<T>>
{
    static bool serialize(Buffer& buffer, const std::vector<T>& data)
    {
        uint32_t len = static_cast<uint32_t>(data.size());
        if (!Serializer<uint32_t>::serialize(buffer, len))
            return false;
        if (!data.empty())
        {
            const size_t dataSize = len * sizeof(T);
            if (buffer.write(data.data(), dataSize) != dataSize)
                return false;
        }
        return true;
    }
    static bool deserialize(Buffer& buffer, std::vector<T>& data)
    {
        uint32_t len = 0;
        if (!Serializer<uint32_t>::deserialize(buffer, len))
            return false;
        data.resize(len);
        if (len > 0)
        {
            const size_t dataSize = len * sizeof(T);
            if (buffer.read(data.data(), dataSize) != dataSize)
                return false;
        }
        return true;
    }
};

template<typename T, size_t N> struct Serializer<std::array<T, N>>
{
    static bool serialize(Buffer& buffer, const std::array<T, N>& data)
    {
        uint32_t len = static_cast<uint32_t>(data.size());
        if (!Serializer<uint32_t>::serialize(buffer, len))
            return false;
        const size_t dataSize = len * sizeof(T);
        if (buffer.write(data.data(), dataSize) != dataSize)
            return false;
        return true;
    }
    static bool deserialize(Buffer& buffer, std::array<T, N>& data)
    {
        uint32_t len = 0;
        if (!Serializer<uint32_t>::deserialize(buffer, len))
            return false;
        if (len != data.size())
            return false;
        const size_t dataSize = len * sizeof(T);
        if (buffer.read(data.data(), dataSize) != dataSize)
            return false;
        return true;
    }
};