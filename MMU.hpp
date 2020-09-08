#pragma once

#include "Types.hpp"
#include "Registers.hpp"

class CPU;

class MMU
{
public:
    
    enum class MemAccessType
    {
        Read, Write
    };
    
    MMU(CPU* cpu) : m_cpu(cpu) { }
    
    template<typename Width>
    Width read(u32 virtual_address)
    {
        return mem_access<Width, MemAccessType::Read>(virtual_address, Width(0));
    }
    template<typename Width>
    void write(u32 virtual_address, Width value)
    {
        mem_access<Width, MemAccessType::Write>(virtual_address, value);
    }
    
    void copy_to_vm(u32 dest, void* src, u32 size);
    void copy_to_host(void* dest, u32 src, u32 size);
    
    
    
protected:
    
    friend class CPU;
    
    template<typename Width, MemAccessType t>
    Width mem_access(u32 virtual_address, Width value);
    
    CPU* m_cpu;
    
    u8 m_physical_ram[0x200000];
    u8 m_expreg1[0x800000];
    u8 m_scrpad[0x400];
    u8 m_hwregs[0x2000];
    u8 m_bios[0x80000];
    u8 m_ioports[0x200];
    
    union
    {
        struct
        {
            u32 indx, rand, tlbl, bpc,  ctxt, bda,   pidmask, dcic,
            badv, bdam, tlbh, bpcm, sr,   cause, epc,     prid, erreg;
        };
        u32 raw[17];
        
        void set(u8 i, u32 value)
        {
            raw[i] = value;
        }
        u32 get(u8 i)
        {
            return raw[i];
        }
        
        u32& operator[](u8 index)
        {
            return raw[index];
        }
    } m_regs;
};
