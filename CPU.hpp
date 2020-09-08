#pragma once

#include "Types.hpp"
#include "PSXExecutable.hpp"
#include "CPUInstruction.hpp"
#include "Registers.hpp"
#include "MMU.hpp"
#include "DMA.hpp"
#include "GPU.hpp"

class CPU
{
public:
    
    enum class Exception
    {
        Interrupt,
        TLBModification,
        TLBLoad,
        TLBStore,
        AddressLoad,
        AddressStore,
        BusInstruction,
        BusData,
        SystemCall,
        Break,
        Reserved,
        COPUnusable,
        Overflow
    };

	CPU() {}

	void init(const char* psxexe_path);
    void run();
	void exec();
	void reset();
    void print();
    void exception(Exception);

protected:
    
    friend class MMU;
    friend class DMA;
    friend class GPU;
    
    /**
     * instruction buffer
     */
    CPUInstruction m_curr_instruction { 0 }; u32 m_curr_pc { 0 };
    
    bool m_slow_mode { false };
    bool m_print_mode { false };
    
    std::pair<u8, u32> m_load_operation { 0, 0 };
    
    bool m_branching            { false };
    bool m_branch_in_delay_slot { false };
    
    /**
     * general purpose registers + internals
     */
    
    struct
    {
        u32 out[32];
        u8  first_last_changed_reg  { 0 };
        u8  second_last_changed_reg { 0 };
        
        union
        {
            struct
            {
                u32 r0, r1, v0, v1, a0, a1, a2, a3,
                    t0, t1, t2, t3, t4, t5, t6, t7,
                    s0, s1, s2, s3, s4, s5, s6, s7,
                    t8, t9, k0, k1, gp, sp, fp, ra, hi, lo, pc, npc;
            };
            u32 raw[36];
        };
        
        void set(u8 i, u32 value)
        {
            out[i] = value;
            out[0] = 0;
            second_last_changed_reg = i;
        }
        
        u32 get(u8 i)
        {
            return raw[i];
        }
        
        u32& operator[](u8 i)
        {
            return raw[i];
        }
    } m_regs;
    
    /**
     * branch handlers
     */
    void branch_set(u32 virtual_address);
    void branch_jmp(u32 virtual_address);
    void branch_add(s16 amount);
    
    /**
     * devices
     */
    MMU m_mmu { this };
    DMA m_dma { this };
    GPU m_gpu { this };
    
    /**
     * base instruction implementation
     */
    void UNK(CPUInstruction&);
    void FUN(CPUInstruction&);
    void B(CPUInstruction&);
    void J(CPUInstruction&);
    void JAL(CPUInstruction&);
    void BEQ(CPUInstruction&);
    void BNE(CPUInstruction&);
    void BLEZ(CPUInstruction&);
    void BGTZ(CPUInstruction&);
    void ADDI(CPUInstruction&);
    void ADDIU(CPUInstruction&);
    void SLTI(CPUInstruction&);
    void SLTIU(CPUInstruction&);
    void ANDI(CPUInstruction&);
    void ORI(CPUInstruction&);
    void XORI(CPUInstruction&);
    void LUI(CPUInstruction&);
    void COP0(CPUInstruction&);
    void COP1(CPUInstruction&);
    void COP2(CPUInstruction&);
    void COP3(CPUInstruction&);
    void LB(CPUInstruction&);
    void LH(CPUInstruction&);
    void LWL(CPUInstruction&);
    void LW(CPUInstruction&);
    void LBU(CPUInstruction&);
    void LHU(CPUInstruction&);
    void LWR(CPUInstruction&);
    void SB(CPUInstruction&);
    void SH(CPUInstruction&);
    void SWL(CPUInstruction&);
    void SW(CPUInstruction&);
    void SWR(CPUInstruction&);
    void LWC0(CPUInstruction&);
    void LWC1(CPUInstruction&);
    void LWC2(CPUInstruction&);
    void LWC3(CPUInstruction&);
    void SWC0(CPUInstruction&);
    void SWC1(CPUInstruction&);
    void SWC2(CPUInstruction&);
    void SWC3(CPUInstruction&);
    
    /**
     * funct instruction implementation
     */
    void SLL(CPUInstruction&);
    void SRL(CPUInstruction&);
    void SRA(CPUInstruction&);
    void SLLV(CPUInstruction&);
    void SRLV(CPUInstruction&);
    void SRAV(CPUInstruction&);
    void JR(CPUInstruction&);
    void JALR(CPUInstruction&);
    void SYSCALL(CPUInstruction&);
    void BREAK(CPUInstruction&);
    void MFHI(CPUInstruction&);
    void MTHI(CPUInstruction&);
    void MFLO(CPUInstruction&);
    void MTLO(CPUInstruction&);
    void MULT(CPUInstruction&);
    void MULTU(CPUInstruction&);
    void DIV(CPUInstruction&);
    void DIVU(CPUInstruction&);
    void ADD(CPUInstruction&);
    void ADDU(CPUInstruction&);
    void SUB(CPUInstruction&);
    void SUBU(CPUInstruction&);
    void AND(CPUInstruction&);
    void OR(CPUInstruction&);
    void XOR(CPUInstruction&);
    void NOR(CPUInstruction&);
    void SLT(CPUInstruction&);
    void SLTU(CPUInstruction&);
    
    typedef void (CPU::*OpHandler)(CPUInstruction&);
    
    /**
     * base instruction handler map
     */
    OpHandler m_base_op_handlers[64] =
    {
        &CPU::FUN, &CPU::B, &CPU::J, &CPU::JAL, &CPU::BEQ, &CPU::BNE, &CPU::BLEZ, &CPU::BGTZ,
        &CPU::ADDI, &CPU::ADDIU, &CPU::SLTI, &CPU::SLTIU, &CPU::ANDI, &CPU::ORI, &CPU::XORI, &CPU::LUI,
        &CPU::COP0, &CPU::COP1, &CPU::COP2, &CPU::COP3, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK,
        &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK,
        &CPU::LB, &CPU::LH, &CPU::LWL, &CPU::LW, &CPU::LBU, &CPU::LHU, &CPU::LWR, &CPU::UNK,
        &CPU::SB, &CPU::SH, &CPU::SWL, &CPU::SW, &CPU::UNK, &CPU::UNK, &CPU::SWR, &CPU::UNK,
        &CPU::LWC0, &CPU::LWC1, &CPU::LWC2, &CPU::LWC3, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK,
        &CPU::SWC0, &CPU::SWC1, &CPU::SWC2, &CPU::SWC3, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK,
    };
    
    /**
     * funct instruction handler map
     */
    OpHandler m_funct_op_handlers[64] =
    {
        &CPU::SLL, &CPU::UNK, &CPU::SRL, &CPU::SRA, &CPU::SLLV, &CPU::UNK, &CPU::SRLV, &CPU::SRAV,
        &CPU::JR, &CPU::JALR, &CPU::UNK, &CPU::UNK, &CPU::SYSCALL, &CPU::BREAK, &CPU::UNK, &CPU::UNK,
        &CPU::MFHI, &CPU::MTHI, &CPU::MFLO, &CPU::MTLO, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK,
        &CPU::MULT, &CPU::MULTU, &CPU::DIV, &CPU::DIVU, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK,
        &CPU::ADD, &CPU::ADDU, &CPU::SUB, &CPU::SUBU, &CPU::AND, &CPU::OR, &CPU::XOR, &CPU::NOR,
        &CPU::UNK, &CPU::UNK, &CPU::SLT, &CPU::SLTU, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK,
        &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK,
        &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK, &CPU::UNK
    };
};
