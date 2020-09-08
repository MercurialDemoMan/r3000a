#include "PSXExecutable.hpp"
#include "File.hpp"

Result PSXExecutable::load(const char* path)
{
    File in(path);
    
    if(in.failed())
    {
        return Result::Bad;
    }
    
    in.read(&m_header, sizeof(Header));
    
    if(memcmp(m_header.magic, "PS-X EXE", 8))
    {
        return Result::Bad;
    }
    
    /*
    std::printf("Initial PC: 0x%08x\n", m_header.pc_init);
    std::printf("Initial GP: 0x%08x\n", m_header.gp_init);
    std::printf("Text Address: 0x%08x\n", m_header.text_address);
    std::printf("Text Size: 0x%08x\n", m_header.text_size);
    std::printf("Data Address: 0x%08x\n", m_header.data_address);
    std::printf("Data Size: 0x%08x\n", m_header.data_size);
    std::printf("Fill Address: 0x%08x\n", m_header.fill_address);
    std::printf("Fill Size: 0x%08x\n", m_header.fill_size);
    std::printf("SP Base: 0x%08x\n", m_header.sp_base);
    std::printf("SP Offset: 0x%0x\n", m_header.sp_offset);
    */
    m_text = new CPUInstruction[m_header.text_size / sizeof(CPUInstruction)];
    
    if(in.read(m_text, m_header.text_size) != m_header.text_size)
    {
        return Result::Bad;
    }
    
    in.close();
    
    return Result::Ok;
}

PSXExecutable::~PSXExecutable()
{
    delete m_text;
}
