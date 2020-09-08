#pragma once

#include "Types.hpp"
#include "Registers.hpp"
#include "GPUInstruction.hpp"
#include "Renderer.hpp"

#include <cstdio>
#include <utility>

class CPU;

class GPU
{
public:
    
    enum class Transparency : u8
    {
        HalfEach    = 0,
        Additive    = 1,
        Subtractive = 2,
        AddQuarted  = 4
    };
    
    enum class TexDepth : u8
    {
        Depth4Bits  = 0,
        Depth8Bits  = 1,
        Depth15Bits = 2,
        Reserved    = 3
    };
    
    enum class Field : u8
    {
        Bot = 0,
        Top = 1
    };
    
    enum class VideoMode : u8
    {
        NTSC = 0,
        PAL  = 1
    };
    
    using HResolution = u8;
    HResolution hres_from_fields(u8 hr1, u8 hr2)
    {
        return (hr2 & 1) | ((hr1 & 3) << 1);
    }
    u32 hres_info_status(HResolution hres)
    {
        return static_cast<u32>(hres) << 16;
    }
    
    enum class VResolution : u8
    {
        Res240ScanLines = 0,
        Res480ScanLines = 1
    };
    
    enum class DisplayDepth : u8
    {
        Depth15Bits = 0,
        Depth24Bits = 1
    };
    
    enum class DMAMode : u8
    {
        Off       = 0,
        Fifo      = 1,
        CPUToGP0  = 2,
        VRAMToCPU = 3
    };

    GPU(CPU* cpu) : m_cpu(cpu)
    {
        m_display_disabled = true;
    }
    
    void set(u8 i, u32 value);
    u32  get(u8 i);
    void gp0_exec(GPUInstruction);
    void gp1_exec(GPUInstruction);
    
protected:
    
    friend class CPU;
    
    CPU* m_cpu;
    
    enum class GP0Mode : u8
    {
        Command,
        Image
    } m_gp0_mode { GP0Mode::Command };
    
    //STAT bits
    u8           m_tex_page_base_x;
    u8           m_tex_page_base_y;
    Transparency m_semi_transparency;
    TexDepth     m_tex_depth;
    bool         m_dithering;
    bool         m_draw_to_display;
    bool         m_force_mask_bit;
    bool         m_preserved_masked_pixels;
    Field        m_field;
    bool         m_tex_disable;
    HResolution  m_h_resolution;
    VResolution  m_v_resolution;
    VideoMode    m_video_mode;
    DisplayDepth m_display_depth;
    bool         m_v_interlace;
    bool         m_display_disabled;
    bool         m_interrupt;
    DMAMode      m_dma_mode;
    
    //GP0 bits
    bool m_rect_tex_x_flip;
    bool m_rect_tex_y_flip;
    
    //GP1 bits
    u8 m_tex_window_x_mask;
    u8 m_tex_window_y_mask;
    u8 m_tex_window_x_offset;
    u8 m_tex_window_y_offset;
    u16 m_drawing_area_left;
    u16 m_drawing_area_top;
    u16 m_drawing_area_right;
    u16 m_drawing_area_bottom;
    s16 m_drawing_x_offset;
    s16 m_drawing_y_offset;
    u16 m_display_vram_x_start;
    u16 m_display_vram_y_start;
    u16 m_display_h_start;
    u16 m_display_h_end;
    u16 m_display_v_start;
    u16 m_display_v_end;
    
    //GP0 instructions
    void GP0_UNK(GPUInstruction&); // unknown instruction
    void GP0_NOP(GPUInstruction&); // unknown instruction (maybe nop?)
    void GP0_CLRCACHE(GPUInstruction&); //clear cache
    void GP0_TEXWIN(GPUInstruction&); // set texture window
    void GP0_MONOQUAD(GPUInstruction&); // draw monochrome quadrilateral
    void GP0_TEXBLENDQUAD(GPUInstruction&); // draw textured quadrilateral with color blending
    void GP0_SHADTRI(GPUInstruction&);  // draw shaded triangle
    void GP0_SHADQUAD(GPUInstruction&); // draw shaded quadrilateral
    void GP0_DRAWMODE(GPUInstruction&); // set GPU flags
    void GP0_DRAWATL(GPUInstruction&);  // set drawing top left area
    void GP0_DRAWABR(GPUInstruction&);  // set drawing bottom right area
    void GP0_DRAWOFF(GPUInstruction&);  // set drawing offset
    void GP0_MASKBIT(GPUInstruction&);  // set mask bit
    void GP0_LDIMAGE(GPUInstruction&);  // load image from ram into vram
    void GP0_STIMAGE(GPUInstruction&);  // store image from vram into ram
    
    //GP1 instructions
    void GP1_UNK(GPUInstruction&); // unknown instruction
    void GP1_CLRFIFO(GPUInstruction&); // clears fifo and resets the gpu
    void GP1_ACKINT(GPUInstruction&); // acknowledge interrupt
    void GP1_RST(GPUInstruction&); // reset GPU flags
    void GP1_DISPLAYDISABLE(GPUInstruction&); // set display disable flag
    void GP1_VRAMSTART(GPUInstruction&); // set VRAM start
    void GP1_HRANGE(GPUInstruction&); // set display horizontal range
    void GP1_VRANGE(GPUInstruction&); // set display vertical range
    void GP1_DMAMODE(GPUInstruction&); // set DMA mode
    void GP1_DISPLAYMODE(GPUInstruction&); // set display mode
    
    typedef void (GPU::*OpHandler)(GPUInstruction&);
    
    std::pair<OpHandler, u32> m_gp0_op_handlers[256] =
    {
        [0 ... 0xFF] = { &GPU::GP0_UNK, 1 },
        [0x00] = { &GPU::GP0_NOP, 1 },
        [0x01] = { &GPU::GP0_CLRCACHE, 1 },
        [0x28] = { &GPU::GP0_MONOQUAD, 5 },
        [0x2C] = { &GPU::GP0_TEXBLENDQUAD, 9 },
        [0x30] = { &GPU::GP0_SHADTRI, 6 },
        [0x38] = { &GPU::GP0_SHADQUAD, 8 },
        [0xE1] = { &GPU::GP0_DRAWMODE, 1 },
        [0xE2] = { &GPU::GP0_TEXWIN, 1 },
        [0xE3] = { &GPU::GP0_DRAWATL, 1 },
        [0xE4] = { &GPU::GP0_DRAWABR, 1 },
        [0xE5] = { &GPU::GP0_DRAWOFF, 1 },
        [0xE6] = { &GPU::GP0_MASKBIT, 1 },
        [0xA0] = { &GPU::GP0_LDIMAGE, 3 },
        [0xC0] = { &GPU::GP0_STIMAGE, 3 },
    };
    
    OpHandler m_gp1_op_handlers[256] =
    {
        [0x00 ... 0xFF] = &GPU::GP1_UNK,
        [0x00] = &GPU::GP1_RST,
        [0x01] = &GPU::GP1_CLRFIFO,
        [0x02] = &GPU::GP1_ACKINT,
        [0x03] = &GPU::GP1_DISPLAYDISABLE,
        [0x04] = &GPU::GP1_DMAMODE,
        [0x05] = &GPU::GP1_VRAMSTART,
        [0x06] = &GPU::GP1_HRANGE,
        [0x07] = &GPU::GP1_VRANGE,
        [0x08] = &GPU::GP1_DISPLAYMODE,
    };
    
    //instruction argument buffer
    class Arguments
    {
    public:
        
        u32& operator[](u8 i)
        {
            assert(i < m_length);
            return m_buffer[i];
        }
        
        void clear()         { m_length = 0; }
        void push(u32 value) { assert(m_length < 12); m_buffer[m_length++] = value; }
        
        u32 length() const { return m_length; }
        
    private:
        
        u32 m_buffer[12]; // longest command has 12 arguments
        u32 m_length { 0 };
        
    } m_gp0_arguments;
    u32 m_gp0_remaining_data { 0 };
    OpHandler m_gp0_queued_handler { nullptr };
    
    //OpenGL renderer
    Renderer m_renderer { 1024, 512 };
};
