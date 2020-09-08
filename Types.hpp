#pragma once

#include <cstdint>
#include <cassert>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

enum class Result : u32
{
    Ok,
    Bad
};

template<typename T1, typename T2>
constexpr T1 bit_cast(T2 v)
{
    static_assert(sizeof(T1) == sizeof(T2));
    
    return *((T1*)&v);
}

template<typename T>
bool constexpr static_is_power_of_two(T v)
{
    return v && !(v & (v - 1));
}

template<typename T>
bool is_power_of_two(T v)
{
    return v && !(v & (v - 1));
}

template<typename T>
bool is_aligned(u32 address)
{
    if constexpr (static_is_power_of_two(sizeof(T)))
    {
        return !(address & (sizeof(T) - 1));
    }
    else
    {
        return !(address % sizeof(T));
    }
}

template<typename T>
bool overflow_add(T x, T y, T z)
{
    return (((z ^ x) & (z ^ y)) & (1 << (sizeof(T) * 8 - 1)));
}

template<typename T>
bool overflow_sub(T x, T y, T z)
{
    return (((z ^ x) & (x ^ y)) & (1 << (sizeof(T) * 8 - 1)));
}
