#pragma once
#include <cstdint>

extern "C"
{
    // IO access functions defined in asmfunc.asm
    void IoOut32(uint16_t addr, uint32_t data);
    uint32_t IoIn32(uint16_t addr);
}