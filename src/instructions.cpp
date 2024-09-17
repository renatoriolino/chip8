#include "instructions.h"

namespace Instructions
{

void Jump(Register<uint64_t> * PC, uint64_t addr)
{
    if (PC)
        *PC = addr;
}

void SkipNext(Register<uint64_t> * PC, bool condition)
{
    if (PC && condition)
        *PC += 2;
}

    
}
