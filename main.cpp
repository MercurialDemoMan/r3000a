#include "CPU.hpp"

#ifdef main
#undef main
#endif

int main(int argc, const char* argv[])
{	
	MARK_AS_USED(argc);
	MARK_AS_USED(argv);
	
    CPU* cpu = new CPU();
    cpu->init(argv[1]);
    cpu->run();
}
