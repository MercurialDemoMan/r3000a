#include "CPU.hpp"
#include "File.hpp"

#include <unordered_map>

//#define DEBUG_MEMORY

#define PRINT_INS(...) if(m_print_mode) { std::printf(__VA_ARGS__); }

void CPU::init(const char* psxexe_path)
{
    PSXExecutable exe(psxexe_path);
    
    m_regs.r0  = 0;
    m_regs.pc  = 0xBFC00000;//exe.pc_init();
    m_regs.npc = m_regs.pc + sizeof(CPUInstruction);
    m_regs.gp  = 0;//exe.gp_init();
    m_regs.sp  = 0;//exe.sp_init();
    
    memcpy(m_regs.out, m_regs.raw, sizeof(m_regs.out));
    
    //load program into ram
    m_mmu.copy_to_vm(exe.text_init(), exe.text_begin(), exe.text_size());

    //initialize bios
    File bios("../scph1001.bin");
    std::vector<u8> bios_content = std::move(bios.read());
    assert(bios_content.size() == 0x80000);
    memcpy(m_mmu.m_bios, &bios_content[0], 0x80000);
    
    //m_print_mode = true;
}

void CPU::run()
{
    while(true)
    {
        //TODO: handle interrupts
        
        exec();
        m_gpu.m_renderer.poll_events();
    }
}

void CPU::exec()
{
    m_curr_pc = m_regs.pc;
    
    if(!is_aligned<u32>(m_curr_pc))
    {
        exception(Exception::AddressLoad);
        return;
    }
    
    m_curr_instruction = m_mmu.read<CPUInstruction>(m_regs.pc);
    
    m_regs.pc   = m_regs.npc;
    m_regs.npc += sizeof(CPUInstruction);
    
    m_branch_in_delay_slot = m_branching;
    m_branching            = false;
    
    m_regs.set(m_load_operation.first, m_load_operation.second);
    m_load_operation.first  = 0;
    m_load_operation.second = 0;
    m_regs.first_last_changed_reg = m_regs.second_last_changed_reg;
    
    /*static u64 executed_ins = 0;
    static bool start_log = false;
    
    if(m_curr_instruction == 0x87a30018)
    {
        start_log = true;
        m_print_mode = true;
    }
    
    if(start_log)
    {
        std::printf("%lld = 0x%08x: [0x%08x] - ", executed_ins, m_curr_pc, m_curr_instruction.raw());
        
        executed_ins++;
    }
    
    if(executed_ins == 100000)
    {
        assert(false);
    }*/
    
    
    (this->*m_base_op_handlers[m_curr_instruction.op()])(m_curr_instruction);
    
    m_regs[m_regs.first_last_changed_reg]  = m_regs.out[m_regs.first_last_changed_reg];
    m_regs[m_regs.second_last_changed_reg] = m_regs.out[m_regs.second_last_changed_reg];
    
    if(m_slow_mode){}
        //usleep(500000);
}

void CPU::print()
{
    for(u32 i = 0; i < 32; i++)
    {
        std::printf("R%d: 0x%08x ", i, m_regs.raw[i]);
        
        if(i != 0 && ((i + 1) % 8) == 0)
        {
            std::printf("\n");
        }
    }
}

void CPU::exception(Exception cause)
{
    static const char* exception_names[] =
    {
        "Interrupt",
        "TLB Modification",
        "TLB Load",
        "TLB Store",
        "Address Error On Load",
        "Address Error On Store",
        "Bus Error Instruction Fetch",
        "Bus Error Data Load / Store",
        "System Call",
        "Break",
        "Reserved",
        "Coprocessor Unusable",
        "Overflow"
    };
    
    PRINT_INS("CPU::exception(): %s\n", exception_names[static_cast<u8>(cause)]);
    
    u32 mode = m_mmu.m_regs.sr & 0x3F;
    
    m_mmu.m_regs.sr &= ~0x3F;
    m_mmu.m_regs.sr |= (mode << 2) & 0x3F;
    
    m_mmu.m_regs.cause &= ~0x7F;
    m_mmu.m_regs.cause  = (static_cast<u32>(cause) << 2);
    
    if(m_branch_in_delay_slot)
    {
        m_mmu.m_regs.epc   -= sizeof(CPUInstruction);
        m_mmu.m_regs.cause |= 1 << 31;
    }
    else
    {
        m_mmu.m_regs.epc    = m_curr_pc;
        m_mmu.m_regs.cause &= ~(1 << 31);
    }
    
    m_regs.pc  = (const u32[2]){ 0x80000080, 0xBFC00180 }[!!(m_mmu.m_regs.sr & (1 << 22))];
    m_regs.npc = m_regs.pc + sizeof(CPUInstruction);
}

void CPU::branch_jmp(u32 virtual_address)
{
    m_regs.npc = (m_regs.pc & 0xF0000000) | (virtual_address * sizeof(CPUInstruction));
    m_branching = true;
}

void CPU::branch_set(u32 virtual_address)
{
    m_regs.npc = virtual_address;
    m_branching = true;
}

void CPU::branch_add(s16 amount)
{
    m_regs.npc = m_regs.pc + (amount * sizeof(CPUInstruction));
    m_branching = true;
}

#define UNIPLEMENTED_INSTRUCTION() std::printf("CPU error: uniplemented instruction 0x%08x at 0x%08x\n", ins.raw(), static_cast<u32>(m_regs.pc - sizeof(CPUInstruction))); assert(false);
#define UNKNOWN_INSTRUCTION() std::printf("CPU error: unknown instruction 0x%08x at 0x%08x\n", ins.raw(), static_cast<u32>(m_regs.pc - sizeof(CPUInstruction))); assert(false);

void CPU::UNK(CPUInstruction& ins)
{
    exception(Exception::Reserved);
    UNKNOWN_INSTRUCTION();
}
void CPU::FUN(CPUInstruction& ins)
{
    (this->*m_funct_op_handlers[ins.funct()])(ins);
}
void CPU::B(CPUInstruction& ins)
{
    PRINT_INS("B\n");
    
    bool is_bgez = (ins.raw() >> 16) & 1;
    bool is_link = ((ins.raw() >> 17) & 0xF) == 8;
    
    u32 test = (static_cast<s32>(m_regs[ins.rs()]) < 0) ^ is_bgez;
    
    if(is_link)
    {
        m_regs.set(31, m_regs.npc);
    }
    
    if(test)
    {
        branch_add(static_cast<s16>(ins.immediate()));
    }
    
}
void CPU::J(CPUInstruction& ins)
{
    PRINT_INS("J      0x%08x[0x%08x]\n", ins.target(), static_cast<u32>((m_regs.pc & 0xF0000000) | (ins.target() * sizeof(CPUInstruction))));
    branch_jmp(ins.target());
}
void CPU::JAL(CPUInstruction& ins)
{
    static const std::unordered_map<u32, std::string> boot_symbols =
    {
        { 0xBFC00000, "boot_init_phase1" },
        { 0xBFC00150, "boot_init_phase2" },
        { 0xBFC06EC4, "boot_init_phase3" },
        { 0xBFC01A60, "trace_step" },
        { 0xBFC03990, "trace_step_bogus" },
        { 0xBFC0703C, "check_pio" },
        { 0xBFC0711C, "init_pio" },
        { 0xBFC06784, "start_kernel" },
        
        { 0xBFC033C8, "strcpy" },
        { 0xBFC03190, "strcat" },
        
        { 0xBFC067E8, "kernel_main" },
        
        { 0xBFC0D850, "clear_stack" }
    };
    
    PRINT_INS("JAL    0x%08x[0x%08x]\n", ins.target(), static_cast<u32>((m_regs.pc & 0xF0000000) | (ins.target() * sizeof(CPUInstruction))));
    
    m_regs.set(static_cast<u8>(GPReg::RA), m_regs.npc);
    
    branch_jmp(ins.target());
    
    /*auto it = boot_symbols.find(m_regs.pc);
    if(it == boot_symbols.end())
    {
        PRINT_INS("0x%08x():\n", m_regs.pc);
    }
    else
    {
        PRINT_INS("0x%08x_%s():\n", m_regs.pc, it->second.c_str());
    }
    */
}
void CPU::BEQ(CPUInstruction& ins)
{
    PRINT_INS("BEQ    R%d[0x%08x] R%d[0x%08x] 0x%04x\n", ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()], ins.immediate());
    if(m_regs[ins.rs()] == m_regs[ins.rt()])
    {
        branch_add(static_cast<s16>(ins.immediate()));
    }
}
void CPU::BNE(CPUInstruction& ins)
{
    PRINT_INS("BNE    R%d[0x%08x] R%d[0x%08x] 0x%04x[0x%08x]\n", ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()], ins.immediate(), static_cast<u32>(m_regs.pc + static_cast<s16>(ins.immediate()) * sizeof(CPUInstruction)));
    if(m_regs[ins.rs()] != m_regs[ins.rt()])
    {
        branch_add(static_cast<s16>(ins.immediate()));
    }
}
void CPU::BLEZ(CPUInstruction& ins)
{
    PRINT_INS("BLEZ   R%d[0x%08x] 0x%04x\n", ins.rs(), m_regs[ins.rs()], ins.immediate());
    if(static_cast<s32>(m_regs[ins.rs()]) <= 0)
    {
        branch_add(static_cast<s16>(ins.immediate()));
    }
}
void CPU::BGTZ(CPUInstruction& ins)
{
    PRINT_INS("BGTZ   R%d[0x%08x] 0x%04x\n", ins.rs(), m_regs[ins.rs()], ins.immediate());
    if(static_cast<s32>(m_regs[ins.rs()]) > 0)
    {
        branch_add(static_cast<s16>(ins.immediate()));
    }
}
void CPU::ADDI(CPUInstruction& ins)
{
    //TODO: write out whats gonna be written into the register
    PRINT_INS("ADDI   R%d[0x%08x] R%d[0x%08x] 0x%04x\n", ins.rt(), m_regs[ins.rt()], ins.rs(), m_regs[ins.rs()], ins.immediate());
    
    u32 result = m_regs[ins.rs()] + static_cast<s16>(ins.immediate());
    
    if(overflow_add<u32>(m_regs[ins.rs()],
                     static_cast<u32>(static_cast<s32>(static_cast<s16>(ins.immediate()))),
                     result))
    {
        exception(Exception::Overflow);
    }
    
    m_regs.set(ins.rt(), result);
}
void CPU::ADDIU(CPUInstruction& ins)
{
    PRINT_INS("ADDIU  R%d[0x%08x] R%d[0x%08x] 0x%04x\n", ins.rt(), m_regs[ins.rt()], ins.rs(), m_regs[ins.rs()], ins.immediate());
    m_regs.set(ins.rt(), m_regs[ins.rs()] +
                             static_cast<s16>(ins.immediate()));
}
void CPU::SLTI(CPUInstruction& ins)
{
    //TODO: are we supposed to overflow?
    PRINT_INS("SLTI   R%d[0x%08x] R%d[0x%08x] 0x%04x\n", ins.rt(), m_regs[ins.rt()], ins.rs(), m_regs[ins.rs()], ins.immediate());
    m_regs.set(ins.rt(), static_cast<s32>(m_regs[ins.rs()]) <
                             static_cast<s16>(ins.immediate()));
}
void CPU::SLTIU(CPUInstruction& ins)
{
    PRINT_INS("SLTIU  R%d[0x%08x] R%d[0x%08x] 0x%04x\n", ins.rt(), m_regs[ins.rt()], ins.rs(), m_regs[ins.rs()], ins.immediate());
    m_regs.set(ins.rt(), (m_regs[ins.rs()]) <
                             static_cast<s16>(ins.immediate()));
}
void CPU::ANDI(CPUInstruction& ins)
{
    PRINT_INS("ANDI   R%d[0x%08x] R%d[0x%08x] 0x%04x\n", ins.rt(), m_regs[ins.rt()], ins.rs(), m_regs[ins.rs()], ins.immediate());
    m_regs.set(ins.rt(), m_regs[ins.rs()] & static_cast<u32>(ins.immediate()));
}
void CPU::ORI(CPUInstruction& ins)
{
    PRINT_INS("ORI    R%d[0x%08x] R%d[0x%08x] 0x%04x\n", ins.rt(), m_regs[ins.rt()], ins.rs(), m_regs[ins.rs()], ins.immediate());
    m_regs.set(ins.rt(), m_regs[ins.rs()] | static_cast<u32>(ins.immediate()));
}
void CPU::XORI(CPUInstruction& ins)
{
    PRINT_INS("XORI   R%d[0x%08x] R%d[0x%08x] 0x%04x\n", ins.rt(), m_regs[ins.rt()], ins.rs(), m_regs[ins.rs()], ins.immediate());
    m_regs.set(ins.rt(), m_regs[ins.rs()] ^ static_cast<u32>(ins.immediate()));
}
void CPU::LUI(CPUInstruction& ins)
{
    PRINT_INS("LUI    R%d[0x%08x] 0x%04x\n", ins.rt(), m_regs[ins.rt()], ins.immediate());
    m_regs.set(ins.rt(), static_cast<u32>(ins.immediate()) << 16);
}
void CPU::COP0(CPUInstruction& ins)
{
    if(ins.is_cop_base_op())
    {
        switch(ins.cop_enum())
        {
            case CPUInstruction::CopOp::MFCN:
            {
                PRINT_INS("MFC0   COP0R%d[0x%08x] R%d[0x%08x]\n", ins.rt(), m_mmu.m_regs[ins.rt()], ins.rd(), m_regs[ins.rd()]);
                
                m_load_operation = { ins.rt(), m_mmu.m_regs[ins.rd()] };
                
                break;
            }
            case CPUInstruction::CopOp::CFCN:
            {
                UNIPLEMENTED_INSTRUCTION(); break;
            }
            case CPUInstruction::CopOp::MTCN:
            {
                PRINT_INS("MTC0   R%d[0x%08x] COP0R%d[0x%08x]\n", ins.rt(), m_regs[ins.rt()], ins.rd(), m_mmu.m_regs[ins.rd()]);
                m_mmu.m_regs.set(ins.rd(), m_regs.get(ins.rt()));
                break;
            }
            case CPUInstruction::CopOp::CTCN:
            {
                UNIPLEMENTED_INSTRUCTION(); break;
            }
            case CPUInstruction::CopOp::BCNC:
            {
                UNIPLEMENTED_INSTRUCTION(); break;
            }
            default:
            {
                UNIPLEMENTED_INSTRUCTION(); break;
            }
        }
    }
    else
    {
        switch(ins.cop0_enum())
        {
            case CPUInstruction::Cop0Op::TLBR:
            {
                UNIPLEMENTED_INSTRUCTION(); break;
            }
            case CPUInstruction::Cop0Op::TLBWI:
            {
                UNIPLEMENTED_INSTRUCTION(); break;
            }
            case CPUInstruction::Cop0Op::TLBWR:
            {
                UNIPLEMENTED_INSTRUCTION(); break;
            }
            case CPUInstruction::Cop0Op::TLBP:
            {
                UNIPLEMENTED_INSTRUCTION(); break;
            }
            case CPUInstruction::Cop0Op::RFE:
            {
                u32 mode = m_mmu.m_regs.sr & 0x3F;
                m_mmu.m_regs.sr &= ~0xF;
                m_mmu.m_regs.sr |= mode >> 2;
                break;
            }
            default:
            {
                UNIPLEMENTED_INSTRUCTION(); break;
            }
        }
    }
}
void CPU::COP1(CPUInstruction& ins)
{
    exception(Exception::COPUnusable);
}
void CPU::COP2(CPUInstruction& ins)
{
    UNIPLEMENTED_INSTRUCTION();
}
void CPU::COP3(CPUInstruction& ins)
{
    exception(Exception::COPUnusable);
}
void CPU::LB(CPUInstruction& ins)
{
    PRINT_INS("LB     R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    m_load_operation = { ins.rt(), static_cast<u32>(static_cast<s32>(m_mmu.read<s8>(m_regs[ins.rs()] + static_cast<s16>(ins.immediate())))) };
}
void CPU::LH(CPUInstruction& ins)
{
    PRINT_INS("LH     R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    m_load_operation = { ins.rt(), static_cast<u32>(static_cast<s32>(m_mmu.read<s16>(m_regs[ins.rs()] + static_cast<s16>(ins.immediate())))) };
    
    //m_print_mode = true;
    //m_slow_mode = true;
}
void CPU::LWL(CPUInstruction& ins)
{
    PRINT_INS("LWL    R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    u32 address         = m_regs[ins.rs()] + static_cast<s16>(ins.immediate());
    u32 aligned_address = address & ~u32(3);
    
    u32 aligned_word    = m_mmu.read<u32>(aligned_address);
    u8  shift           = (address & 3) * 8;
    
    m_load_operation = { ins.rt(), (m_regs[ins.rt()] & (0x00FFFFFF >> shift)) | (aligned_word << (24 - shift)) };
}
void CPU::LW(CPUInstruction& ins)
{
    PRINT_INS("LW     R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    m_load_operation = { ins.rt(), m_mmu.read<u32>(m_regs[ins.rs()] + static_cast<s16>(ins.immediate())) };
}
void CPU::LBU(CPUInstruction& ins)
{
    PRINT_INS("LBU    R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    m_load_operation =  { ins.rt(), m_mmu.read<u8>(m_regs[ins.rs()] +
                                             static_cast<s16>(ins.immediate())) };
}
void CPU::LHU(CPUInstruction& ins)
{
    PRINT_INS("LHU    R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    m_load_operation = { ins.rt(), m_mmu.read<u16>(m_regs[ins.rs()] + static_cast<s16>(ins.immediate())) };
}
void CPU::LWR(CPUInstruction& ins)
{
    PRINT_INS("LWR    R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    u32 address         = m_regs[ins.rs()] + static_cast<s16>(ins.immediate());
    u32 aligned_address = address & ~u32(3);
    
    u32 aligned_word    = m_mmu.read<u32>(aligned_address);
    u8  shift           = (address & 3) * 8;
    
    m_load_operation = { ins.rt(), (m_regs[ins.rt()] & (0xFFFFFF00 << (24 - shift))) | (aligned_word >> shift) };
}
void CPU::SB(CPUInstruction& ins)
{
    PRINT_INS("SB     R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    m_mmu.write<u8>(m_regs[ins.rs()] + static_cast<s16>(ins.immediate()), static_cast<u8>(m_regs[ins.rt()] & 0xFF));
}
void CPU::SH(CPUInstruction& ins)
{
    PRINT_INS("SH     R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    m_mmu.write<u16>(m_regs[ins.rs()] + static_cast<s16>(ins.immediate()), static_cast<u16>(m_regs[ins.rt()] & 0xFFFF));
}
void CPU::SWL(CPUInstruction& ins)
{
    PRINT_INS("SWL    R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    u32 address         = m_regs[ins.rs()] + static_cast<s16>(ins.immediate());
    u32 aligned_address = address & ~u32(3);
    u32 value_to_write  = m_regs[ins.rt()];
    
    u32 aligned_word    = m_mmu.read<u32>(aligned_address);
    u8  shift           = (address & 3) * 8;
    
    value_to_write      = (aligned_word & (0xFFFFFF00 << shift)) | (value_to_write >> (24 - shift));
    
    m_mmu.write<u32>(aligned_address, value_to_write);
}
void CPU::SW(CPUInstruction& ins)
{
    PRINT_INS("SW     R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    m_mmu.write<u32>(m_regs[ins.rs()] + static_cast<s16>(ins.immediate()), m_regs[ins.rt()]);
}
void CPU::SWR(CPUInstruction& ins)
{
    PRINT_INS("SWR    R%d[0x%08x] 0x%04x (R%d[0x%08x])\n", ins.rt(), m_regs[ins.rt()], ins.immediate(), ins.rs(), m_regs[ins.rs()]);
    
    u32 address         = m_regs[ins.rs()] + static_cast<s16>(ins.immediate());
    u32 aligned_address = address & ~u32(3);
    u32 value_to_write  = m_regs[ins.rt()];
    
    u32 aligned_word    = m_mmu.read<u32>(aligned_address);
    u8  shift           = (address & 3) * 8;
    
    value_to_write      = (aligned_word & (0x00FFFFFF >> (24 - shift))) | (value_to_write << shift);
    
    m_mmu.write<u32>(aligned_address, value_to_write);
}
void CPU::LWC0(CPUInstruction& ins)
{
    exception(Exception::COPUnusable);
}
void CPU::LWC1(CPUInstruction& ins)
{
    exception(Exception::COPUnusable);
}
void CPU::LWC2(CPUInstruction& ins)
{
    UNIPLEMENTED_INSTRUCTION();
}
void CPU::LWC3(CPUInstruction& ins)
{
    exception(Exception::COPUnusable);
}
void CPU::SWC0(CPUInstruction& ins)
{
    exception(Exception::COPUnusable);
}
void CPU::SWC1(CPUInstruction& ins)
{
    exception(Exception::COPUnusable);
}
void CPU::SWC2(CPUInstruction& ins)
{
    UNIPLEMENTED_INSTRUCTION();
}
void CPU::SWC3(CPUInstruction& ins)
{
    exception(Exception::COPUnusable);
}
void CPU::SLL(CPUInstruction& ins)
{
    PRINT_INS("SLL    R%d[0x%08x] R%d[0x%08x] 0x%02x\n", ins.rd(), m_regs[ins.rd()], ins.rt(), m_regs[ins.rt()], ins.shamt());
    m_regs.set(ins.rd(), m_regs[ins.rt()] << ins.shamt());
}
void CPU::SRL(CPUInstruction& ins)
{
    PRINT_INS("SRL    R%d[0x%08x] R%d[0x%08x] 0x%02x\n", ins.rd(), m_regs[ins.rd()], ins.rt(), m_regs[ins.rt()], ins.shamt());
    m_regs.set(ins.rd(), m_regs[ins.rt()] >> ins.shamt());
}
void CPU::SRA(CPUInstruction& ins)
{
    PRINT_INS("SRA    R%d[0x%08x] R%d[0x%08x] 0x%02x\n", ins.rd(), m_regs[ins.rd()], ins.rt(), m_regs[ins.rt()], ins.shamt());
    m_regs.set(ins.rd(), static_cast<u32>(static_cast<s32>(m_regs[ins.rt()]) >> ins.shamt()));
}
void CPU::SLLV(CPUInstruction& ins)
{
    PRINT_INS("SLLV   R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rt(), m_regs[ins.rt()], ins.rs(), m_regs[ins.rs()]);
    m_regs.set(ins.rd(), m_regs[ins.rt()] << (m_regs[ins.rs()] & 0b11111));
}
void CPU::SRLV(CPUInstruction& ins)
{
    PRINT_INS("SRLV   R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rt(), m_regs[ins.rt()], ins.rs(), m_regs[ins.rs()]);
    m_regs.set(ins.rd(), m_regs[ins.rt()] >> (m_regs[ins.rs()] & 0b11111));
}
void CPU::SRAV(CPUInstruction& ins)
{
    PRINT_INS("SRAV   R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rt(), m_regs[ins.rt()], ins.rs(), m_regs[ins.rs()]);
    m_regs.set(ins.rd(), static_cast<u32>(static_cast<s32>(m_regs[ins.rt()]) >> (m_regs[ins.rs()] & 0b11111)));
}
void CPU::JR(CPUInstruction& ins)
{
    PRINT_INS("JR     R%d[0x%08x]\n", ins.rs(), m_regs[ins.rs()]);
    /*if(ins.raw() == 0x03e00008)
    {
        PRINT_INS("\n");
    }*/
    branch_set(m_regs[ins.rs()]);
}
void CPU::JALR(CPUInstruction& ins)
{
    PRINT_INS("JALR   R%d[0x%08x] R%d[0x%08x]\n", ins.rs(), m_regs[ins.rs()], ins.rd(), m_regs[ins.rd()]);
    
    u32 ra = m_regs.npc;
    
    branch_set(m_regs[ins.rs()]);
    
    m_regs.set(ins.rd(), ra);
}
void CPU::SYSCALL(CPUInstruction& ins)
{
    PRINT_INS("SYSCALL\n");
    exception(Exception::SystemCall);
}
void CPU::BREAK(CPUInstruction& ins)
{
    PRINT_INS("BREAK\n");
    exception(Exception::Break);
}
void CPU::MFHI(CPUInstruction& ins)
{
    PRINT_INS("MFHI   R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()]);
    m_regs.set(ins.rd(), m_regs.hi);
}
void CPU::MTHI(CPUInstruction& ins)
{
    PRINT_INS("MTHI   R%d[0x%08x]\n", ins.rs(), m_regs[ins.rs()]);
    m_regs.hi = m_regs[ins.rs()];
}
void CPU::MFLO(CPUInstruction& ins)
{
    PRINT_INS("MFLO   R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()]);
    m_regs.set(ins.rd(), m_regs.lo);
}
void CPU::MTLO(CPUInstruction& ins)
{
    PRINT_INS("MTLO   R%d[0x%08x]\n", ins.rs(), m_regs[ins.rs()]);
    m_regs.lo = m_regs[ins.rs()];
}
void CPU::MULT(CPUInstruction& ins)
{
    PRINT_INS("MULT   R%d[0x%08x] R%d[0x%08x]\n", ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    u64 res = static_cast<u64>(static_cast<s64>(static_cast<s32>(m_regs[ins.rs()])) *
                               static_cast<s64>(static_cast<s32>(m_regs[ins.rt()])));
    
    m_regs.lo = static_cast<u32>((res >>  0) & 0xFFFFFFFF);
    m_regs.hi = static_cast<u32>((res >> 32) & 0xFFFFFFFF);
}
void CPU::MULTU(CPUInstruction& ins)
{
    PRINT_INS("MULTU  R%d[0x%08x] R%d[0x%08x]\n", ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    u64 res = static_cast<u64>(m_regs[ins.rs()]) *
              static_cast<u64>(m_regs[ins.rt()]);
    
    m_regs.lo = static_cast<u32>((res >>  0) & 0xFFFFFFFF);
    m_regs.hi = static_cast<u32>((res >> 32) & 0xFFFFFFFF);
}
void CPU::DIV(CPUInstruction& ins)
{
    PRINT_INS("DIV    R%d[0x%08x] R%d[0x%08x]\n", ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    
    if(m_regs[ins.rt()] == 0)
    {
        m_regs.hi = m_regs[ins.rs()];
        m_regs.lo = static_cast<s32>(m_regs[ins.rs()]) >= 0 ? 0xFFFFFFFF : 1;
    }
    else if(m_regs[ins.rs()] == 0x80000000 && m_regs[ins.rt()] == 0xFFFFFFFF)
    {
        m_regs.lo = 0x80000000;
        m_regs.hi = 0;
    }
    else
    {
        m_regs.lo = static_cast<u32>(static_cast<s32>(m_regs[ins.rs()]) / static_cast<s32>(m_regs[ins.rt()]));
        m_regs.hi = static_cast<u32>(static_cast<s32>(m_regs[ins.rs()]) % static_cast<s32>(m_regs[ins.rt()]));
    }
}
void CPU::DIVU(CPUInstruction& ins)
{
    PRINT_INS("DIVU   R%d[0x%08x] R%d[0x%08x]\n", ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    
    if(m_regs[ins.rt()] == 0)
    {
        m_regs.hi = m_regs[ins.rs()];
        m_regs.lo = 0xFFFFFFFF;
    }
    else
    {
        m_regs.lo = m_regs[ins.rs()] / m_regs[ins.rt()];
        m_regs.hi = m_regs[ins.rs()] % m_regs[ins.rt()];
    }
}
void CPU::ADD(CPUInstruction& ins)
{
    PRINT_INS("ADD    R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    
    u32 result = m_regs[ins.rs()] + m_regs[ins.rt()];
    
    if(overflow_add<u32>(m_regs[ins.rs()],
                         m_regs[ins.rt()],
                         result))
    {
        exception(Exception::Overflow);
    }
    
    m_regs.set(ins.rd(), result);
}
void CPU::ADDU(CPUInstruction& ins)
{
    PRINT_INS("ADDU   R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    m_regs.set(ins.rd(), m_regs[ins.rs()] +
                             m_regs[ins.rt()]);
}
void CPU::SUB(CPUInstruction& ins)
{
    PRINT_INS("SUB    R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    
    u32 result = m_regs[ins.rs()] - m_regs[ins.rt()];
    
    if(overflow_sub<u32>(m_regs[ins.rs()],
                         m_regs[ins.rt()],
                         result))
    {
        exception(Exception::Overflow);
    }
    
    m_regs.set(ins.rd(), result);
}
void CPU::SUBU(CPUInstruction& ins)
{
    PRINT_INS("SUBU   R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    m_regs.set(ins.rd(), m_regs[ins.rs()] -
                             m_regs[ins.rt()]);
}
void CPU::AND(CPUInstruction& ins)
{
    PRINT_INS("AND    R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    m_regs.set(ins.rd(), m_regs[ins.rs()] & m_regs[ins.rt()]);
}
void CPU::OR(CPUInstruction& ins)
{
    PRINT_INS("OR     R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    m_regs.set(ins.rd(), m_regs[ins.rs()] | m_regs[ins.rt()]);
}
void CPU::XOR(CPUInstruction& ins)
{
    PRINT_INS("XOR    R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    m_regs.set(ins.rd(), m_regs[ins.rs()] ^ m_regs[ins.rt()]);
}
void CPU::NOR(CPUInstruction& ins)
{
    PRINT_INS("NOR    R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    m_regs.set(ins.rd(), ~(m_regs[ins.rs()] | m_regs[ins.rt()]));
}
void CPU::SLT(CPUInstruction& ins)
{
    PRINT_INS("SLT    R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    m_regs.set(ins.rd(), static_cast<s32>(m_regs[ins.rs()]) <
                             static_cast<s32>(m_regs[ins.rt()]));
}
void CPU::SLTU(CPUInstruction& ins)
{
    PRINT_INS("SLTU   R%d[0x%08x] R%d[0x%08x] R%d[0x%08x]\n", ins.rd(), m_regs[ins.rd()], ins.rs(), m_regs[ins.rs()], ins.rt(), m_regs[ins.rt()]);
    m_regs.set(ins.rd(), m_regs[ins.rs()] <
                             m_regs[ins.rt()]);
}
