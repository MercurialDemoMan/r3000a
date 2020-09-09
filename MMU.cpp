#include "MMU.hpp"

#include "CPU.hpp"

void MMU::copy_to_vm(u32 destination, void* source, u32 size)
{
    u8* byte_src = reinterpret_cast<u8*>(source);
    for(u32 i = 0; i < size; i++)
    {
        write<u8>(destination + i, byte_src[i]);
    }
}

void MMU::copy_to_host(void* destination, u32 source, u32 size)
{
    u8* byte_dest = reinterpret_cast<u8*>(destination);
    for(u32 i = 0; i < size; i++)
    {
        byte_dest[i] = read<u8>(source + i);
    }
}

template<typename Width, MMU::MemAccessType t>
Width MMU::mem_access(u32 virtual_address, Width value)
{
#define RW(place) \
if constexpr(t == MemAccessType::Read) { return place; } else { return place = value; }
    
#define RWREG(regs, reg) \
if constexpr(t == MemAccessType::Read) { return regs.get(static_cast<u8>(reg)); } else { regs.set(static_cast<u8>(reg), value); }
    
    assert(m_cpu != nullptr);
    
    if constexpr (sizeof(Width) != 1)
    {
        if(!is_aligned<Width>(virtual_address))
        {
            std::printf("MMU::mem_access() error: unaligned address [0x%08x -> %u]\n", virtual_address, sizeof(Width));
            if constexpr (t == MemAccessType::Read)
            {
                m_cpu->exception(CPU::Exception::AddressLoad);
            }
            else
            {
                m_cpu->exception(CPU::Exception::AddressStore);
            }
        }
    }
    
    if constexpr (t == MemAccessType::Write)
    {
        if(m_regs.sr & COP0_RS_ISOLATE_CACHE) //cpu is directly targeting the cache -> ignore writes
        {
            return Width(0);
        }
    }
    
    switch(virtual_address)
    {
        case 0x00000000 ... 0x001FFFFF:
        case 0x80000000 ... 0x801FFFFF:
        case 0xA0000000 ... 0xA01FFFFF: // Kernel / User Space
        {
            u32 physical_address = virtual_address & 0x001FFFFF;
            
            RW(*((Width*)(m_physical_ram + physical_address)));
            break;
        }
            
        case 0x9F000000 ... 0x9F7FFFFF:
        case 0xBF000000 ... 0xBF7FFFFF:
        case 0x1F000000 ... 0x1F7FFFFF: // Expansion Region 1
        {
            u32 physical_address = virtual_address & 0x007FFFFF;
			
			MARK_AS_USED(physical_address);
            
            if constexpr (t == MemAccessType::Read)
            {
                return Width(0xFF);
            }
            else
            {

            }
            break;
        }
            
        case 0x9F800000 ... 0x9F8003FF:
        case 0xBF800000 ... 0xBF8003FF:
        case 0x1F800000 ... 0x1F8003FF: // Scratch Pad
        {
            u32 physical_address = virtual_address & 0x000003FF;
            
            RW(*((Width*)(m_scrpad + physical_address)));
            break;
        }
            
        case 0x9F801000 ... 0x9F802FFF:
        case 0xBF801000 ... 0xBF802FFF:
        case 0x1F801000 ... 0x1F802FFF: // Hardware Registers
        {
            u32 physical_address = virtual_address - 0x1F801000;
            
            switch(physical_address)
            {
                case 0: //expansion 1 base address
                {
                    if constexpr (t == MemAccessType::Write)
                        assert(u32(value) == 0x1F000000);
                    break;
                }
                    
                case 4: //expansion 2 base address
                {
                    if constexpr (t == MemAccessType::Write)
                        assert(u32(value) == 0x1F802000);
                    break;
                }
                    
                case 8: //expansion 1 size
                {
                    break;
                }
                    
                case 0x0C: // expansion 3 size
                {
                    break;
                }
                    
                case 0x10: // BIOS
                {
                    break;
                }
                    
                case 0x14: // SPU size
                {
                    break;
                }
                    
                case 0x18: // CDROM size
                {
                    break;
                }
                    
                case 0x1C: // expansion 2 size
                {
                    break;
                }
                    
                case 0x20: // unknown
                {
                    break;
                }
                    
                case 0x60: // RAM size
                {
                    break;
                }
                    
                case 0x100 ... 0x12F: // Timer
                {
                    break;
                }
                    
                case 0xC00 ... 0xE80: //SPU
                {
                    break;
                }
                    
                case 0x70 ... 0x78: //IRQ
                {
                    break;
                }
                
                case 0x80: /* DMA mdecin base channel */ { RWREG(m_cpu->m_dma, DMAReg::CH0_MDECIN_BASE); break; }
                case 0x84: /* DMA mdecin block channel */ { RWREG(m_cpu->m_dma, DMAReg::CH0_MDECIN_BLOCK); break; }
                case 0x88: /* DMA mdecin control channel */ { RWREG(m_cpu->m_dma, DMAReg::CH0_MDECIN_CTRL); break; }
                case 0x90: /* DMA mdecout base channel */ { RWREG(m_cpu->m_dma, DMAReg::CH1_MDECOUT_BASE); break; }
                case 0x94: /* DMA mdecout block channel */ { RWREG(m_cpu->m_dma, DMAReg::CH1_MDECOUT_BLOCK); break; }
                case 0x98: /* DMA mdecout control channel */ { RWREG(m_cpu->m_dma, DMAReg::CH1_MDECOUT_CTRL); break; }
                case 0xA0: /* DMA gpu base channel */ { RWREG(m_cpu->m_dma, DMAReg::CH2_GPU_BASE); break; }
                case 0xA4: /* DMA gpu block channel */ { RWREG(m_cpu->m_dma, DMAReg::CH2_GPU_BLOCK); break; }
                case 0xA8: /* DMA gpu control channel */ { RWREG(m_cpu->m_dma, DMAReg::CH2_GPU_CTRL); break; }
                case 0xB0: /* DMA cdrom base channel */ { RWREG(m_cpu->m_dma, DMAReg::CH3_CDROM_BASE); break; }
                case 0xB4: /* DMA cdrom block channel */ { RWREG(m_cpu->m_dma, DMAReg::CH3_CDROM_BLOCK); break; }
                case 0xB8: /* DMA cdrom control channel */ { RWREG(m_cpu->m_dma, DMAReg::CH3_CDROM_CTRL); break; }
                case 0xC0: /* DMA spu base channel */ { RWREG(m_cpu->m_dma, DMAReg::CH4_SPU_BASE); break; }
                case 0xC4: /* DMA spu block channel */ { RWREG(m_cpu->m_dma, DMAReg::CH4_SPU_BLOCK); break; }
                case 0xC8: /* DMA spu control channel */ { RWREG(m_cpu->m_dma, DMAReg::CH4_SPU_CTRL); break; }
                case 0xD0: /* DMA pio base channel */ { RWREG(m_cpu->m_dma, DMAReg::CH5_PIO_BASE); break; }
                case 0xD4: /* DMA pio block channel */ { RWREG(m_cpu->m_dma, DMAReg::CH5_PIO_BLOCK); break; }
                case 0xD8: /* DMA pio control channel */ { RWREG(m_cpu->m_dma, DMAReg::CH5_PIO_CTRL); break; }
                case 0xE0: /* DMA otc base channel */ { RWREG(m_cpu->m_dma, DMAReg::CH6_OTC_BASE); break; }
                case 0xE4: /* DMA otc block channel */ { RWREG(m_cpu->m_dma, DMAReg::CH6_OTC_BLOCK); break; }
                case 0xE8: /* DMA otc control channel */ { RWREG(m_cpu->m_dma, DMAReg::CH6_OTC_CTRL); break; }
                case 0xF0: /* DMA control */ { RWREG(m_cpu->m_dma, DMAReg::CTRL); break; }
                case 0xF4: /* DMA interrupt */ { RWREG(m_cpu->m_dma, DMAReg::INT); break; }
                    
                case 0x810: //GP0 / GPUREAD
                {
                    RWREG(m_cpu->m_gpu, GPUReg::GP0_READ) break;
                }
                case 0x814: //GP1 / GPUSTAT
                {
                    RWREG(m_cpu->m_gpu, GPUReg::GP1_STAT); break;
                }
                    
                case 0x1041: // expansion 2 post
                {
                    break;
                }
                    
                default:
                    std::printf("MMU::mem_access(): unhandled address [0x%08x] - [0x%08x]\n", virtual_address, physical_address);
                    //assert(false);
            }
            
            break;
        }
            
        case 0xFFFE0000 ... 0xFFFE01FF: // I/O Ports
        {
            u32 physical_address = virtual_address & 0x000001FF;
            
            RW(*((Width*)(m_ioports + physical_address)));
            break;
        }
            
        case 0x1FC00000 ... 0x1FC7FFFF:
        case 0x9FC00000 ... 0x9FC7FFFF:
        case 0xBFC00000 ... 0xBFC7FFFF: // BIOS
        {
            u32 physical_address = virtual_address & 0x0007FFFF;
            
            if constexpr (t == MemAccessType::Read)
            {
                return *((Width*)(m_bios + physical_address));
            }
            else //read only
            {
                return Width(0);
            }
            break;
        }
            
        default:
        {
            m_cpu->print();
            std::printf("MMU::mem_access<>() error: unknown address [0x%08x]\n", virtual_address);
            if constexpr (t == MemAccessType::Read)
            {
                m_cpu->exception(CPU::Exception::AddressLoad);
            }
            else
            {
                m_cpu->exception(CPU::Exception::AddressStore);
            }
            break;
        }
    }
    
    return Width(0);
    
#undef RW
#undef RWREG
}

template u8 MMU::mem_access<u8, MMU::MemAccessType::Read>(u32, u8);
template u16 MMU::mem_access<u16, MMU::MemAccessType::Read>(u32, u16);
template u32 MMU::mem_access<u32, MMU::MemAccessType::Read>(u32, u32);
template CPUInstruction MMU::mem_access<CPUInstruction, MMU::MemAccessType::Read>(u32, CPUInstruction);

template u8 MMU::mem_access<u8, MMU::MemAccessType::Write>(u32, u8);
template u16 MMU::mem_access<u16, MMU::MemAccessType::Write>(u32, u16);
template u32 MMU::mem_access<u32, MMU::MemAccessType::Write>(u32, u32);
template CPUInstruction MMU::mem_access<CPUInstruction, MMU::MemAccessType::Write>(u32, CPUInstruction);

template s8 MMU::mem_access<s8, MMU::MemAccessType::Read>(u32, s8);
template s16 MMU::mem_access<s16, MMU::MemAccessType::Read>(u32, s16);
template s32 MMU::mem_access<s32, MMU::MemAccessType::Read>(u32, s32);

template s8 MMU::mem_access<s8, MMU::MemAccessType::Write>(u32, s8);
template s16 MMU::mem_access<s16, MMU::MemAccessType::Write>(u32, s16);
template s32 MMU::mem_access<s32, MMU::MemAccessType::Write>(u32, s32);
