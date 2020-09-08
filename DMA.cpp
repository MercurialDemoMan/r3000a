#include "DMA.hpp"
#include "CPU.hpp"

template<DMA::DMAChannel channel_type>
void DMA::execute(DMA::Channel& channel)
{
    //assert(channel.sync != Sync::LList);
    
    //direct block transfer
    if(channel.sync != Sync::LList)
    {
        std::printf("DMA Log: Block DMA start - %s\n", channel.mode == Mode::FromRAM ? "From RAM" : "To RAM");
        
        u32 base  = channel.base;
        u32 total = channel.transfer_size();
        u32 dir   = (channel.direction == Direction::Inc ? 1 : -1) * 4;
        u32 src;
        
        while(total > 0)
        {
            u32 address = base & 0x1FFFFC;
            
            if(channel.mode == Mode::FromRAM)
            {
                src = m_cpu->m_mmu.read<u32>(address);
                
                if constexpr (channel_type == DMAChannel::GPU)
                {
                    m_cpu->m_gpu.gp0_exec(src);
                }
                else
                {
                    assert(false);
                }
            }
            else
            {
                if constexpr (channel_type == DMAChannel::OTC)
                {
                    if(total == 1)       { src = 0xFFFFFF; }
                    else { src = (address - 4) & 0x1FFFFF; }
                }
                else if constexpr (channel_type == DMAChannel::GPU)
                {
                    assert(false);
                    //src = m_cpu->m_gpu.get(static_cast<u8>(GPUReg::GP0_READ));
                }
                else
                {
                    assert(false);
                }
                
                
                m_cpu->m_mmu.write<u32>(address, src);
            }
            
            base += dir;
            total--;
        }
    }
    //transfer blocks within a linked list blocks
    else
    {
        u32 address = channel.base & 0x1FFFFC;
        
        std::printf("DMA Log: LList DMA start\n");
        
        if(channel.mode == Mode::FromRAM)
        {
            //is linked list gpu only?
            assert(channel_type == DMAChannel::GPU);
            
            for(;;)
            {
                u32 header = m_cpu->m_mmu.read<u32>(address);
                
                u32 total = header >> 24;
                
                while(total > 0)
                {
                    address = (address + 4) & 0x1FFFFC;
                    
                    u32 cmd = m_cpu->m_mmu.read<u32>(address);
                    m_cpu->m_gpu.gp0_exec(cmd); //send gpu command to the gpu
                    total--;
                }
                
                if(header & 0x800000) //0xFFFFFF
                {
                    break;
                }
                
                address = header & 0x1FFFFC;
            }
        }
        else
        {
            assert(false);
        }
    }
    
    //TODO: set bits to end transfer
    channel.enabled = false;
    channel.trigger = false;
    
    //assert(false);
}

template void DMA::execute<DMA::DMAChannel::MDECIN>(DMA::Channel& channel);
template void DMA::execute<DMA::DMAChannel::MDECOUT>(DMA::Channel& channel);
template void DMA::execute<DMA::DMAChannel::GPU>(DMA::Channel& channel);
template void DMA::execute<DMA::DMAChannel::CDROM>(DMA::Channel& channel);
template void DMA::execute<DMA::DMAChannel::SPU>(DMA::Channel& channel);
template void DMA::execute<DMA::DMAChannel::PIO>(DMA::Channel& channel);
template void DMA::execute<DMA::DMAChannel::OTC>(DMA::Channel& channel);
