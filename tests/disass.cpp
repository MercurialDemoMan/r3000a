#include "../PSXExecutable.hpp"

int main(int argc, const char* argv[])
{
    PSXExecutable e;
    e.load(argv[1]);
    
    for(u32 i = 0; i < e.text_size() / sizeof(Instruction); i++)
    {
        Instruction ins = *(e.text_begin() + i);
        std::printf("0x%08x : ", e.text_init() + i * sizeof(Instruction));
        ins.print();
    }
    
    return 0;
}
