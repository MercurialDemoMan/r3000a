#pragma once

#include <cstdio>

#include "Types.hpp"
#include "Registers.hpp"

class CPUInstruction
{
public:
    
    CPUInstruction() {}
    CPUInstruction(u32 v) : m_raw(v) {}
    
    CPUInstruction& operator=(u32 v)
    {
        m_raw = v;
        return *this;
    }
    
    CPUInstruction& operator=(CPUInstruction& i)
    {
        m_raw = i.raw();
        return *this;
    }
    
    operator u32() const
    {
        return m_raw;
    }
    
    enum class BaseOp : u8
    {
        Funct = 0,
        B = 1,
        J = 2,
        JAL = 3,
        BEQ = 4,
        BNE = 5,
        BLEZ = 6,
        BGTZ = 7,
        ADDI = 8,
        ADDIU = 9,
        SLTI = 10,
        SLTIU = 11,
        ANDI = 12,
        ORI = 13,
        XORI = 14,
        LUI = 15,
        COP0 = 16,
        COP1 = 17,
        COP2 = 18,
        COP3 = 19,
        LB = 32,
        LH = 33,
        LWL,
        LW,
        LBU,
        LHU,
        LWR,
        SB = 40,
        SH,
        SWL,
        SW,
        SWR = 46,
        LWC0 = 48,
        LWC1,
        LWC2,
        LWC3,
        SWC0 = 56,
        SWC1,
        SWC2,
        SCWC3
    };
    
    enum class FunctOp : u8
    {
        SLL = 0,
        SRL = 2,
        SRA = 3,
        SLLV = 4,
        SRLV = 6,
        SRAV = 7,
        JR = 8,
        JALR = 9,
        SYSCALL = 12,
        BREAK = 13,
        MFHI = 16,
        MTHI = 17,
        MFLO = 18,
        MTLO = 19,
        MULT = 24,
        MULTU = 25,
        DIV = 26,
        DIVU = 27,
        ADD = 32,
        ADDU,
        SUB,
        SUBU,
        AND,
        OR,
        XOR,
        NOR,
        SH = 41,
        SLT,
        SLTU
    };
    
    enum class CopOp : u8
    {
        MFCN = 0,
        CFCN = 2,
        MTCN = 4,
        CTCN = 6,
        BCNC = 8
    };
    
    enum class Cop0Op : u8
    {
        TLBR  = 1,
        TLBWI = 2,
        TLBWR = 4,
        TLBP  = 8,
        RFE   = 16
    };
    
    static constexpr const char* OpBaseMap[64] =
    {
        "FUN", "B", "J", "JAL", "BEQ", "BNE", "BLEZ", "BGTZ",
        "ADDI", "ADDIU", "SLTI", "SLTIU", "ANDI", "ORI", "XORI", "LUI",
        "COP0", "COP1", "COP2", "COP3", "UNK", "UNK", "UNK", "UNK",
        "UNK", "UNK", "UNK", "UNK", "UNK", "UNK", "UNK", "UNK",
        "LB", "LH", "LWL", "LW", "LBU", "LHU", "LWR", "UNK",
        "SB", "SH", "SWL", "SW", "UNK", "UNK", "SWR", "UNK",
        "LWC0", "LWC1", "LWC2", "LWC3", "UNK", "UNK", "UNK", "UNK",
        "SWC0", "SWC1", "SWC2", "SWC3", "UNK", "UNK", "UNK", "UNK"
    };
    
    static constexpr const char* OpSpecialMap[64] =
    {
        "SLL", "UNK", "SRL", "SRA", "SLLV", "UNK", "SRLV", "SRAV",
        "JR", "JALR", "UNK", "UNK", "SYSCALL", "BREAK", "UNK", "UNK",
        "MFHI", "MTHI", "MFLO", "MTLO", "UNK", "UNK", "UNK", "UNK",
        "MULT", "MULTU", "DIV", "DIVU", "UNK", "UNK", "UNK", "UNK",
        "ADD", "ADDU", "SUB", "SUBU", "AND", "OR", "XOR", "NOR",
        "UNK", "UNK", "SLT", "SLTU", "UNK", "UNK", "UNK", "UNK",
        "UNK", "UNK", "UNK", "UNK", "UNK", "UNK", "UNK", "UNK",
        "UNK", "UNK", "UNK", "UNK", "UNK", "UNK", "UNK", "UNK"
    };
    
    u32     raw()         const { return m_raw; }
    BaseOp  op_enum()     const { return static_cast<BaseOp>((m_raw >> 26) & 0x3F); }
    u8      op()          const { return static_cast<u8>((m_raw >> 26) & 0x3F); }
    GPReg   rs_enum()     const { return static_cast<GPReg>((m_raw >> 21) & 0x1F); }
    u8      rs()          const { return static_cast<u8>((m_raw >> 21) & 0x1F); }
    GPReg   rt_enum()     const { return static_cast<GPReg>((m_raw >> 16) & 0x1F); }
    u8      rt()          const { return static_cast<u8>((m_raw >> 16) & 0x1F); }
    u16     immediate()   const { return static_cast<u16>((m_raw >> 0) & 0xFFFF); }
    
    u32     target()      const { return (m_raw >> 0) & 0x3FFFFFF; }
    
    GPReg   rd_enum()     const { return static_cast<GPReg>((m_raw >> 11) & 0x1F); }
    u8      rd()          const { return static_cast<u8>((m_raw >> 11) & 0x1F); }
    u8      shamt()       const { return static_cast<u8>((m_raw >> 6) & 0x1F); }
    FunctOp funct_enum()  const { return static_cast<FunctOp>((m_raw >> 0) & 0x3F); }
    u8      funct()       const { return static_cast<u8>((m_raw >> 0) & 0x3F); }
    
    u8      cop_type()    const { return static_cast<u8>((m_raw >> 26) & 0x03); }
    u8      cop()         const { return static_cast<u8>((m_raw >> 21) & 0x0F); }
    CopOp   cop_enum()    const { return static_cast<CopOp>((m_raw >> 21) & 0x0F); }
    u8      cop0()        const { return static_cast<u8>((m_raw >> 0) & 0x3F); }
    Cop0Op  cop0_enum()   const { return static_cast<Cop0Op>((m_raw >> 0) & 0x3F); }
    u16     immediate16() const { return static_cast<u16>((m_raw >> 0) & 0xFFFF); }
    u32     immediate25() const { return (m_raw >> 0) & 0x1FFFFFF; }
    
    bool is_cop_base_op() const
    {
        return !(m_raw & (1 << 25));
    }
    
    bool is_jump() const
    {
        BaseOp cached_op = op_enum();
        
        if(cached_op != BaseOp::Funct)
        {
            return cached_op == BaseOp::B    || cached_op == BaseOp::BEQ  || cached_op == BaseOp::BNE ||
                   cached_op == BaseOp::BLEZ || cached_op == BaseOp::BGTZ || cached_op == BaseOp::J   || cached_op == BaseOp::JAL;
        }
        else
        {
            FunctOp cached_funct_op = funct_enum();
            return cached_funct_op == FunctOp::JR || cached_funct_op == FunctOp::JALR;
        }
    }
    
    void print()
    {
        if(op_enum() == BaseOp::Funct)
        {
            std::printf("%s\n", OpSpecialMap[static_cast<u8>(funct())]);
        }
        else
        {
            std::printf("%s\n", OpBaseMap[static_cast<u8>(op())]);
        }
    }
    
private:
    u32  m_raw;
};

static_assert(sizeof(CPUInstruction) == 4);
