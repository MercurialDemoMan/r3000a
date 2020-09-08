#include "../PSXExecutable.hpp"

int main(int argc, const char* argv[])
{
    PSXExecutable e;
    e.load(argv[1]);
    
    return 0;
}
