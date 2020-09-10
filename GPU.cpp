#include "GPU.hpp"

void GPU::set(u8 i, u32 value)
{
    switch(static_cast<GPUReg>(i))
    {
        case GPUReg::GP0_READ: { gp0_exec(value); break; }
        case GPUReg::GP1_STAT: { gp1_exec(value); break; }
            
        default:
        {
            assert(false); break;
        }
    }
}

u32 GPU::get(u8 i)
{
    switch(static_cast<GPUReg>(i))
    {
        case GPUReg::GP0_READ:
        {
            //TODO: implement this
            return 0; break;
        }
        case GPUReg::GP1_STAT:
        {
            u32 value =
            (m_tex_page_base_x << 0) |
            (m_tex_page_base_y << 4) |
            (static_cast<u32>(m_semi_transparency) << 5) |
            (static_cast<u32>(m_tex_depth) << 7) |
            (m_dithering << 9) |
            (m_draw_to_display << 10) |
            (m_force_mask_bit << 11) |
            (m_preserved_masked_pixels << 12) |
            (static_cast<u32>(m_field) << 13) |
            (m_tex_disable << 15) |
            hres_info_status(m_h_resolution) |
            //TODO: implement interlace
            //(static_cast<u32>(m_v_resolution) << 19) |
            (static_cast<u32>(m_video_mode) << 20) |
            (static_cast<u32>(m_display_depth) << 21) |
            (m_v_interlace << 22) |
            (m_display_disabled << 23) |
            (m_interrupt << 24) |
            (1 << 26) |
            (1 << 27) |
            (1 << 28) |
            (static_cast<u32>(m_dma_mode) << 29);
            
            switch(m_dma_mode)
            {
                case DMAMode::Off:       { value |= 0 << 25; break; }
                case DMAMode::Fifo:      { value |= 1 << 25; break; }
                case DMAMode::CPUToGP0:  { value |= ((value >> 28) & 1) << 25; break; }
                case DMAMode::VRAMToCPU: { value |= ((value >> 27) & 1) << 25; break; }
                default:
                {
                    assert(false); break;
                }
            }
            
            return value; break;
        }
            
        default:
        {
            assert(false); break;
        }
    }
}

void GPU::gp0_exec(GPUInstruction command)
{
    static bool print_args = false;
    
    //GP0_LDIMAGE will modify the m_gp0_mode so that it will calculate m_gp0_remaining_data
    if(m_gp0_mode == GP0Mode::Image)
    {
        //TODO: copy pixels
        
        m_gp0_remaining_data--;
        
        if(m_gp0_remaining_data == 0)
        {
            m_gp0_mode = GP0Mode::Command;
        }
    }
    else /* if(m_gp0_mode == GP0Mode::Command) */
    {
        //load the initial command
        if(m_gp0_queued_handler == nullptr)
        {
            std::printf("GP0 cmd: 0x%08x\n", u32(command));
            
            const std::pair<OpHandler, u32>& cmd = m_gp0_op_handlers[command.op()];
            
            m_gp0_queued_handler = cmd.first;
            m_gp0_remaining_data = cmd.second;
            
            print_args = m_gp0_remaining_data != 1;
            
            //skip loading 1 word commands since it will get passed as an argument to the command
            if(m_gp0_remaining_data == 1)
            {
                m_gp0_remaining_data--;
            }
        }
        
        //if command has arguments load them one after each other
        if(m_gp0_remaining_data != 0)
        {
            if(print_args)
                std::printf("GP0 arg: 0x%08x\n", u32(command));
            
            m_gp0_arguments.push(command);
            m_gp0_remaining_data--;
        }
        
        
        //we are done loading arguments -> run the command
        if(m_gp0_remaining_data == 0)
        {
            (this->*m_gp0_queued_handler)(command);
            m_gp0_queued_handler = nullptr;
            m_gp0_arguments.clear();
        }
    }
}

void GPU::gp1_exec(GPUInstruction command)
{
    std::printf("GP1 cmd: 0x%08x\n", u32(command));
    (this->*m_gp1_op_handlers[command.op()])(command);
}

#define UNIPLEMENTED_GP0_INSTRUCTION() std::printf("GPU error: uniplemented GP0 instruction 0x%08x\n", ins.raw()); assert(false);
#define UNIPLEMENTED_GP1_INSTRUCTION() std::printf("GPU error: uniplemented GP1 instruction 0x%08x\n", ins.raw()); assert(false);

//GP0 instructions
void GPU::GP0_UNK(GPUInstruction& ins)
{
    UNIPLEMENTED_GP0_INSTRUCTION();
}
void GPU::GP0_NOP(GPUInstruction& ins)
{
    MARK_AS_USED(ins);
}
void GPU::GP0_CLRCACHE(GPUInstruction& ins)
{
    std::printf("GPU Warning: Clear Texture Cache unimplemented\n");
    MARK_AS_USED(ins);
}
void GPU::GP0_TEXWIN(GPUInstruction& ins)
{
    m_tex_window_x_mask   = (ins >>  0) & 0x1F;
    m_tex_window_y_mask   = (ins >>  5) & 0x1F;
    m_tex_window_x_offset = (ins >> 10) & 0x1F;
    m_tex_window_y_offset = (ins >> 15) & 0x1F;
}
void GPU::GP0_MONOQUAD(GPUInstruction&)
{
    Renderer::Vertex vertices[4] =
    {
        Renderer::Vertex(m_gp0_arguments[1]),
        Renderer::Vertex(m_gp0_arguments[2]),
        Renderer::Vertex(m_gp0_arguments[3]),
		Renderer::Vertex(m_gp0_arguments[4])
    };
    
    Renderer::Color colors[4] =
    {
        Renderer::Color(m_gp0_arguments[0]),
        Renderer::Color(m_gp0_arguments[0]),
        Renderer::Color(m_gp0_arguments[0]),
		Renderer::Color(m_gp0_arguments[0])
    };
    
    m_renderer.draw_shaded_quad(vertices, colors);
}
void GPU::GP0_TEXBLENDQUAD(GPUInstruction&)
{
    std::printf("GPU Warning: Draw Textured Quad With Blending unimplemented\n");
	
	Renderer::Vertex vertices[4] =
    {
        Renderer::Vertex(m_gp0_arguments[1]),
        Renderer::Vertex(m_gp0_arguments[3]),
        Renderer::Vertex(m_gp0_arguments[5]),
		Renderer::Vertex(m_gp0_arguments[7])
    };
    
    Renderer::Color colors[4] =
    {
        Renderer::Color(0x80, 0, 0),
        Renderer::Color(0x80, 0, 0),
        Renderer::Color(0x80, 0, 0),
		Renderer::Color(0x80, 0, 0)
    };
    
    m_renderer.draw_shaded_quad(vertices, colors);
}
void GPU::GP0_SHADTRI(GPUInstruction&)
{
    Renderer::Vertex vertices[3] =
    {
        Renderer::Vertex(m_gp0_arguments[1]),
        Renderer::Vertex(m_gp0_arguments[3]),
        Renderer::Vertex(m_gp0_arguments[5])
    };
    
    Renderer::Color colors[3] =
    {
        Renderer::Color(m_gp0_arguments[0]),
        Renderer::Color(m_gp0_arguments[2]),
        Renderer::Color(m_gp0_arguments[4])
    };
    
    m_renderer.draw_shaded_triangle(vertices, colors);
}
void GPU::GP0_SHADQUAD(GPUInstruction&)
{
    Renderer::Vertex vertices[4] =
    {
        Renderer::Vertex(m_gp0_arguments[1]),
        Renderer::Vertex(m_gp0_arguments[3]),
        Renderer::Vertex(m_gp0_arguments[5]),
		Renderer::Vertex(m_gp0_arguments[7])
    };
    
    Renderer::Color colors[4] =
    {
        Renderer::Color(m_gp0_arguments[0]),
        Renderer::Color(m_gp0_arguments[2]),
        Renderer::Color(m_gp0_arguments[4]),
		Renderer::Color(m_gp0_arguments[6])
    };
    
    m_renderer.draw_shaded_quad(vertices, colors);
}
void GPU::GP0_MONOTRI(GPUInstruction&) { } // draw monochrome triangle
void GPU::GP0_MONOTRANSTRI(GPUInstruction&) { } // draw transparent monochrome triangle
void GPU::GP0_MONOTRANSQUAD(GPUInstruction&) { } // draw transparent monochrome quadrilateral
void GPU::GP0_TEXBLENDTRI(GPUInstruction&) { } // draw textured triangle with blending
void GPU::GP0_TEXRAWTRI(GPUInstruction&) { } // draw textured triangle
void GPU::GP0_TEXBLENDTRANSTRI(GPUInstruction&) { } // draw textured transparent triangle with blending
void GPU::GP0_TEXRAWTRANSTRI(GPUInstruction&) { } // draw textured transparent triangle
void GPU::GP0_TEXRAWQUAD(GPUInstruction&) { } // draw textured quadrilateral
void GPU::GP0_TEXBLENDTRANSQUAD(GPUInstruction&) { } // draw textured transparent quad with blending
void GPU::GP0_TEXRAWTRANSQUAD(GPUInstruction&) { } // draw textured transparent quadrilateral
void GPU::GP0_SHADTRANSTRI(GPUInstruction&) { } // draw shaded transparent triangle
void GPU::GP0_SHADTRANSQUAD(GPUInstruction&) { } // draw shaded transparent quadrilateral
void GPU::GP0_SHADTEXBLENDTRI(GPUInstruction&) { } // draw shaded textured triangle with blending
void GPU::GP0_SHADTEXRAWTRI(GPUInstruction&) { } // draw shaded textured triangle
void GPU::GP0_SHADTEXBLENDQUAD(GPUInstruction&) { } // draw shaded textured quadrilateral with blending
void GPU::GP0_SHADTEXRAWQUAD(GPUInstruction&) { } // draw shaded textured quadrilateral
void GPU::GP0_MONOLINE(GPUInstruction&) { } // draw monochrome line
void GPU::GP0_MONOTRANSLINE(GPUInstruction&) { } // draw monochrome transparent line
void GPU::GP0_MONOPOLYLINE(GPUInstruction&) { } // draw monochrome multiline
void GPU::GP0_MONOTRANSPOLYLINE(GPUInstruction&) { }
void GPU::GP0_SHADLINE(GPUInstruction&) { }
void GPU::GP0_SHADTRANSLINE(GPUInstruction&) { }
void GPU::GP0_SHADPOLYLINE(GPUInstruction&) { }
void GPU::GP0_SHADTRANSPOLYLINE(GPUInstruction&) { }
void GPU::GP0_MONORECT(GPUInstruction&) { }
void GPU::GP0_MONOTRANSRECT(GPUInstruction&) { }
void GPU::GP0_MONORECT1X1(GPUInstruction&) { }
void GPU::GP0_MONOTRANSRECT1X1(GPUInstruction&) { }
void GPU::GP0_MONORECT8X8(GPUInstruction&) { }
void GPU::GP0_MONOTRANSRECT8X8(GPUInstruction&) { }
void GPU::GP0_MONORECT16X16(GPUInstruction&) { }
void GPU::GP0_MONOTRANSRECT16X16(GPUInstruction&) { }
void GPU::GP0_TEXBLENDRECT(GPUInstruction&) { }
void GPU::GP0_TEXRAWRECT(GPUInstruction&) { }
void GPU::GP0_TEXBLENDTRANSRECT(GPUInstruction&) { }
void GPU::GP0_TEXRAWTRANSRECT(GPUInstruction&) { }
void GPU::GP0_TEXBLENDRECT1X1(GPUInstruction&) { }
void GPU::GP0_TEXRAWRECT1X1(GPUInstruction&) { }
void GPU::GP0_TEXBLENDTRANSRECT1X1(GPUInstruction&) { }
void GPU::GP0_TEXRAWTRANSRECT1X1(GPUInstruction&) { }
void GPU::GP0_TEXBLENDRECT8X8(GPUInstruction&) { }
void GPU::GP0_TEXRAWRECT8X8(GPUInstruction&) { }
void GPU::GP0_TEXBLENDTRANSRECT8X8(GPUInstruction&) { }
void GPU::GP0_TEXRAWTRANSRECT8X8(GPUInstruction&) { }
void GPU::GP0_TEXBLENDRECT16X16(GPUInstruction&) { }
void GPU::GP0_TEXRAWRECT16X16(GPUInstruction&) { }
void GPU::GP0_TEXBLENDTRANSRECT16X16(GPUInstruction&) { }
void GPU::GP0_TEXRAWTRANSRECT16X16(GPUInstruction&) { }
void GPU::GP0_DRAWMODE(GPUInstruction& ins)
{
    m_tex_page_base_x   = (ins >> 0) & 0b1111;
    m_tex_page_base_y   = (ins >> 4) & 1;
    m_semi_transparency = static_cast<Transparency>((ins >> 5) & 0b11);
    m_tex_depth         = static_cast<TexDepth>((ins >> 7) & 0b11);
    m_dithering         = (ins >> 9) & 1;
    m_draw_to_display   = (ins >> 10) & 1;
    m_tex_disable       = (ins >> 11) & 1;
    m_rect_tex_x_flip   = (ins >> 12) & 1;
    m_rect_tex_y_flip   = (ins >> 13) & 1;
}
void GPU::GP0_DRAWATL(GPUInstruction& ins)
{
    m_drawing_area_top = (ins >> 10) & 0x3FF;
    m_drawing_area_left = (ins & 0x3FF);
}
void GPU::GP0_DRAWABR(GPUInstruction& ins)
{
    m_drawing_area_bottom = (ins >> 10) & 0x3FF;
    m_drawing_area_right  = ins & 0x3FF;
}
void GPU::GP0_MASKBIT(GPUInstruction& ins)
{
    m_force_mask_bit          = ins & 1;
    m_preserved_masked_pixels = ins & 2;
}
void GPU::GP0_DRAWOFF(GPUInstruction& ins)
{
    u16 x = (ins >>  0) & 0x7FF;
    u16 y = (ins >> 11) & 0x7FF;
    
    m_drawing_x_offset = (static_cast<s16>(x << 5)) >> 5;
    m_drawing_y_offset = (static_cast<s16>(y << 5)) >> 5;
    
    m_renderer.draw();
}
void GPU::GP0_LDIMAGE(GPUInstruction&)
{
    u32 resolution = m_gp0_arguments[2];
    
    u16 width  = (resolution >>  0) & 0xFFFF;
    u16 height = (resolution >> 16) & 0xFFFF;
    
    u32 image_size = (width * height + 1) & ~u32(1);
    
    m_gp0_remaining_data = image_size / 2;
    
    m_gp0_mode = GP0Mode::Image;
}
void GPU::GP0_STIMAGE(GPUInstruction&)
{
    std::printf("GPU Warning: Store Image unimplemented\n");
}

//GP1 instructions
void GPU::GP1_UNK(GPUInstruction& ins)
{
    UNIPLEMENTED_GP1_INSTRUCTION();
}
void GPU::GP1_CLRFIFO(GPUInstruction&)
{
    //TODO: clear fifo
    
    m_gp0_arguments.clear();
    m_gp0_remaining_data = 0;
    m_gp0_mode = GP0Mode::Command;
}
void GPU::GP1_ACKINT(GPUInstruction&)
{
    m_interrupt = false;
}
void GPU::GP1_RST(GPUInstruction&)
{
    m_interrupt = 0;
    m_tex_page_base_x = 0;
    m_tex_page_base_y = 0;
    m_semi_transparency = Transparency::HalfEach;
    m_tex_depth = TexDepth::Depth4Bits;
    m_tex_window_x_mask = 0;
    m_tex_window_y_mask = 0;
    m_tex_window_x_offset = 0;
    m_tex_window_y_offset = 0;
    m_dithering = 0;
    m_draw_to_display = 0;
    m_tex_disable = 0;
    m_rect_tex_x_flip = 0;
    m_rect_tex_y_flip = 0;
    m_drawing_area_left = 0;
    m_drawing_area_top = 0;
    m_drawing_area_right = 0;
    m_drawing_area_bottom = 0;
    m_drawing_x_offset = 0;
    m_drawing_y_offset = 0;
    m_force_mask_bit = 0;
    m_preserved_masked_pixels = 0;
    m_dma_mode = DMAMode::Off;
    m_display_disabled = true;
    m_display_vram_x_start = 0;
    m_display_vram_y_start = 0;
    m_h_resolution = hres_from_fields(0, 0);
    m_v_resolution = VResolution::Res240ScanLines;
    m_video_mode = VideoMode::NTSC;
    m_v_interlace = true;
    m_display_h_start = 0x200;
    m_display_h_end = 0xC00;
    m_display_v_start = 0x10;
    m_display_v_end = 0x100;
    m_display_depth = DisplayDepth::Depth15Bits;
    
    //TODO: shit
}
void GPU::GP1_DISPLAYDISABLE(GPUInstruction& ins)
{
    m_display_disabled = ins & 1;
}
void GPU::GP1_VRAMSTART(GPUInstruction& ins)
{
    m_display_vram_x_start = (ins >>  0) & 0x3FE;
    m_display_vram_y_start = (ins >> 10) & 0x1FF;
}
void GPU::GP1_HRANGE(GPUInstruction& ins)
{
    m_display_h_start = (ins >>  0) & 0xFFF;
    m_display_h_end   = (ins >> 12) & 0xFFF;
}
void GPU::GP1_VRANGE(GPUInstruction& ins)
{
    m_display_v_start = (ins >>  0) & 0x3FF;
    m_display_v_end   = (ins >> 12) & 0x3FF;
}
void GPU::GP1_DMAMODE(GPUInstruction& ins)
{
    m_dma_mode = static_cast<DMAMode>(ins & 0b11);
}
void GPU::GP1_DISPLAYMODE(GPUInstruction& ins)
{
    m_h_resolution = hres_from_fields(ins & 0b11, (ins >> 6) & 1);
    m_v_resolution = static_cast<VResolution>(!!(ins & 0x4));

    m_video_mode = static_cast<VideoMode>(!!(ins & 0x8));
    
    //TODO: this bit flip is weird
    m_display_depth = static_cast<DisplayDepth>(!(ins & 0x10));
    
    m_v_interlace = ins & 0x20;
    
    //unsuported display mode
    assert(!(ins & 0x80));
}
