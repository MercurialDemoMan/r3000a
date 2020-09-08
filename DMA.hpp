#pragma once

#include "Types.hpp"
#include "Registers.hpp"

#include <functional>
#include <cstdio>

class CPU;
class MMU;

class DMA
{
public:
    
    enum class DMAChannel : u8
    {
        MDECIN,
        MDECOUT,
        GPU,
        CDROM,
        SPU,
        PIO,
        OTC
    };
    
    enum class Mode : u8
    {
        ToRAM   = 0,
        FromRAM = 1
    };
    
    enum class Direction : u8
    {
        Inc = 0,
        Dec = 1
    };
    
    enum class Sync : u8
    {
        Manual  = 0,
        Request = 1,
        LList   = 2
    };
    
    struct Channel
    {
        //base register handler
        u32 base;
        
        void base_set(u32 value)
        {
            std::printf("base: 0x%08x\n", value);
            base = value & 0xFFFFF;
        }
        
        u32 base_get()
        {
            return base;
        }
        
        //block register handler
        u16 block_words;
        u16 block_count;
        
        void block_set(u32 value)
        {
            std::printf("block: 0x%08x\n", value);
            block_words = (value >>  0) & 0xFFFF;
            block_count = (value >> 16) & 0xFFFF;
        }
        
        u32 block_get()
        {
            return (block_count << 16) | block_words;
        }
        
        //control register handler
        Mode mode;
        Direction direction;
        bool chop;
        Sync sync;
        u8 chop_dma_window;
        u8 chop_cpu_window;
        bool enabled;
        bool trigger;
        
        void control_set(u32 value)
        {
            mode      = static_cast<Mode>     ((value >>  0) & 1);
            direction = static_cast<Direction>((value >>  1) & 1);
            chop      = (value >> 2) & 1;
            sync      = static_cast<Sync>     ((value >> 9) & 0b11);
            chop_dma_window = (value >> 16) & 0b111;
            chop_cpu_window = (value >> 20) & 0b111;
            enabled   = (value >> 24) & 1;
            trigger   = (value >> 28) & 1;
            
            std::printf("ctrl: 0x%08x, active: %s\n", value, active() ? "true" : "false");
        }
        
        u32 control_get()
        {
            return (static_cast<u32>(mode) << 0) |
                   (static_cast<u32>(direction) << 1) |
                   (chop << 2) |
                   (static_cast<u32>(sync) << 9) |
                   (chop_dma_window << 16) |
                   (chop_cpu_window << 20) |
                   (enabled << 24) |
                   (trigger << 28);
        }
        
        //utility
        bool active() { return enabled && (sync == Sync::Manual ? trigger : true); }
        u32  transfer_size()
        {
            switch(sync)
            {
                case Sync::Manual:  { return block_words; break; }
                case Sync::Request: { return block_words * block_count; break; }
                case Sync::LList:   { return 0; break; }
                default:            { assert(false); break; }
            }
        }
    };
    
    DMA(CPU* cpu) { m_regs.control = 0x07654321; m_cpu = cpu; }
    
    //control
    u32  control() const { return m_regs.control; }
    
    //interrupt
    u32  irq()                const { return m_regs.interrupt; }
    bool irq_en()             const { return (m_regs.interrupt >> 23) & 1; }
    u8   irq_channels()       const { return (m_regs.interrupt >> 16) & 0b1111111; }
    bool irq_force()          const { return (m_regs.interrupt >> 15) & 1; }
    u8   irq_channels_reset() const { return (m_regs.interrupt >> 24) & 0b1111111; }
    bool irq_active()         const { return (m_regs.interrupt >> 31) & 1; }
    
    u32 get(u8 i)
    {
        if(i < static_cast<u8>(DMAReg::CTRL))
        {
            u8 channel_index = i / 3;
            
            if((i % 3) == 0) //alter base
            {
                return m_channels[channel_index].base_get();
            }
            else if((i % 3) == 1) //alter block
            {
                return m_channels[channel_index].block_get();
            }
            else if((i % 3) == 2) //alter control
            {
                return m_channels[channel_index].control_get();
            }
            else
            {
                assert(false);
            }
        }
        return m_regs[i - 21];
    }
    
    void set(u8 i, u32 value)
    {
        if(i < static_cast<u8>(DMAReg::CTRL)) //access channel registers
        {
            u8 channel_index = i / 3;
            if((i % 3) == 0) //alter base
            {
                std::printf("channel: %hhu - ", channel_index);
                m_channels[channel_index].base_set(value);
            }
            else if((i % 3) == 1) //alter block
            {
                std::printf("channel: %hhu - ", channel_index);
                m_channels[channel_index].block_set(value);
            }
            else if((i % 3) == 2) //alter control
            {
                std::printf("channel: %hhu - ", channel_index);
                m_channels[channel_index].control_set(value);
                
                //TODO: fire of dma transfer better
                //      see Nocash: Commonly used DMA Control Register values for starting DMA transfers
                if(m_channels[channel_index].active())
                {
                    switch(channel_index)
                    {
                        case 0: { execute<DMAChannel::MDECIN>(m_channels[channel_index]); break; }
                        case 1: { execute<DMAChannel::MDECOUT>(m_channels[channel_index]); break; }
                        case 2: { execute<DMAChannel::GPU>(m_channels[channel_index]); break; }
                        case 3: { execute<DMAChannel::CDROM>(m_channels[channel_index]); break; }
                        case 4: { execute<DMAChannel::SPU>(m_channels[channel_index]); break; }
                        case 5: { execute<DMAChannel::PIO>(m_channels[channel_index]); break; }
                        case 6: { execute<DMAChannel::OTC>(m_channels[channel_index]); break; }
                        default:
                        {
                            assert(false); break;
                        }
                    }
                }
            }
            else
            {
                std::printf("%hhu\n", i);
                assert(false);
            }
        }
        else
        {
            m_regs[i - 21] = value;
            
            if(static_cast<DMAReg>(i) == DMAReg::INT) //reset channels
            {
                m_regs[i - 21] = ((~value) & 0x007F0000) | (m_regs[static_cast<u8>(DMAReg::INT) & ~0x007F0000]);
            }
        }
    }
    
protected:
    
    template<DMA::DMAChannel channel_type>
    void execute(Channel&);
    
    friend class CPU;
    friend class MMU;
    
    union
    {
        struct
        {
            u32 control, interrupt,
                unknown0, unknown1;
        };
        u32 raw[4];
        
        u32& operator[](u8 i)
        {
            return raw[i];
        }
    } m_regs;
    
    Channel m_channels[7];
    CPU*    m_cpu;
};
