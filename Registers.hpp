#pragma once

#include "Types.hpp"

enum class GPReg : u8
{
    R0 = 0, AT, V0, V1, A0, A1, A2, A3,
    T0, T1, T2, T3, T4, T5, T6, T7,
    S0, S1, S2, S3, S4, S5, S6, S7,
    T8, T9, K0, K1, GP, SP, S8, RA, HI, LO, PC, NPC
};

enum class COP0Reg : u8
{
    INDX     = 0,
    RAND     = 1,
    TLBL     = 2,
    BPC      = 3,
    CTXT     = 4,
    BDA      = 5,
    PIDMASK  = 6,
    DCIC     = 7,
    BADV     = 8,
    BDAM     = 9,
    TLBH     = 10,
    BPCM     = 11,
    SR       = 12,
    CAUSE    = 13,
    EPC      = 14,
    PRID     = 15,
    ERREG    = 16
};

#define COP0_RS_ISOLATE_CACHE 0x00010000

enum class DMAReg : u8
{
    CH0_MDECIN_BASE = 0,
    CH0_MDECIN_BLOCK = 1,
    CH0_MDECIN_CTRL = 2,
    CH1_MDECOUT_BASE = 3,
    CH1_MDECOUT_BLOCK = 4,
    CH1_MDECOUT_CTRL = 5,
    CH2_GPU_BASE = 6,
    CH2_GPU_BLOCK = 7,
    CH2_GPU_CTRL = 8,
    CH3_CDROM_BASE = 9,
    CH3_CDROM_BLOCK = 10,
    CH3_CDROM_CTRL = 11,
    CH4_SPU_BASE = 12,
    CH4_SPU_BLOCK = 13,
    CH4_SPU_CTRL = 14,
    CH5_PIO_BASE = 15,
    CH5_PIO_BLOCK = 16,
    CH5_PIO_CTRL = 17,
    CH6_OTC_BASE = 18,
    CH6_OTC_BLOCK = 19,
    CH6_OTC_CTRL = 20,
    CTRL = 21,
    INT = 22,
    UNK0 = 23,
    UNK1 = 24
};

#define CHANNEL_CTRL_ENABLE 0x1000000

enum class GPUReg : u8
{
    GP0_READ, //write GP0 - read READ
    GP1_STAT, //write GP1 - read STAT
};
