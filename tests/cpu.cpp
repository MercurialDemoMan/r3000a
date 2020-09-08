#include "../CPU.hpp"

#ifdef main
#undef main
#endif

int main(int argc, const char* argv[])
{
    CPU* cpu = new CPU();
    
    cpu->init(argv[1]);
    cpu->run();
}
