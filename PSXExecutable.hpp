#pragma once

#include <string>
#include "Types.hpp"
#include "CPUInstruction.hpp"

class PSXExecutable
{
public:
    
    PSXExecutable() {}
    PSXExecutable(const char* path)
    {
        load(path);
    }
    PSXExecutable(const std::string&& path)
    {
        load(path.c_str());
    }
    
    ~PSXExecutable();
    
    Result load(const std::string&& path)
    {
        return load(path.c_str());
    }
    Result load(const char* path);
    
    u32 pc_init()   const { return m_header.pc_init; }
    u32 gp_init()   const { return m_header.gp_init; }
    u32 text_init() const { return m_header.text_address; }
    u32 text_size() const { return m_header.text_size; }
    u32 sp_init()   const { return m_header.sp_base + m_header.sp_offset; }
    
    CPUInstruction* text_begin() const { return m_text; }
    CPUInstruction* text_end()   const { return m_text + m_header.text_size; }
    
private:
    
    struct Header
    {
        char magic[8];
        char padding0[8];
        u32  pc_init;
        u32  gp_init;
        u32  text_address;
        u32  text_size;
        u32  data_address;
        u32  data_size;
        u32  fill_address;
        u32  fill_size;
        u32  sp_base;
        u32  sp_offset;
        u32  padding1[5];
        char sony_sign[0x7B4];
    } __attribute__((packed));
    
    static_assert(sizeof(Header) == 0x800);
    
    Header          m_header;
    CPUInstruction* m_text { nullptr };
};
