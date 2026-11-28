#if !defined(BITMASK_H)
#define BITMASK_H

#include <Arduino.h>

/**
 * Helpers to split 32-bit values across two 16-bit Modbus registers.
 *
 * Word order is big-endian (high word first), the usual Modbus convention:
 *   regs[0] = bits 31..16, regs[1] = bits 15..0
 * The master must decode with the same order.
 */
class BitMask
{
public:
    static constexpr uint32_t MASK_16 = 0xFFFF;

    static uint16_t highWord(uint32_t value)
    {
        return static_cast<uint16_t>((value >> 16) & MASK_16);
    }

    static uint16_t lowWord(uint32_t value)
    {
        return static_cast<uint16_t>(value & MASK_16);
    }

    // Write value into regs[0] (high) and regs[1] (low)
    static void encodeU32(uint32_t value, uint16_t *regs)
    {
        regs[0] = highWord(value);
        regs[1] = lowWord(value);
    }

    // Rebuild value from regs[0] (high) and regs[1] (low)
    static uint32_t decodeU32(const uint16_t *regs)
    {
        return (static_cast<uint32_t>(regs[0]) << 16) | static_cast<uint32_t>(regs[1]);
    }
};

#endif // BITMASK_H
