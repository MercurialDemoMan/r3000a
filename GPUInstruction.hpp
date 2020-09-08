#pragma once

#include "Types.hpp"

class GPUInstruction
{
public:
    
    GPUInstruction() {}
    GPUInstruction(u32 v) : m_raw(v) {}
    
    GPUInstruction& operator=(u32 v)
    {
        m_raw = v;
        return *this;
    }
    
    operator u32() const
    {
        return m_raw;
    }
    
    u32 raw() const { return m_raw; }
    u8  op()  const { return (m_raw >> 24) & 0xFF; }
    
private:
    
    u32 m_raw;
};
