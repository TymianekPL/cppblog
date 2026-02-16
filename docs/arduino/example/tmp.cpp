#include <inttypes.h>
#include <stddef.h>

constexpr static uintptr_t RIOBegin = 0x00;
constexpr static uintptr_t MMIOBegin = 0x20;

struct MCURegister
{
     uint8_t reserved0 : 1;
     uint8_t bods : 1;
     uint8_t bodse : 1;
     mutable uint8_t pud : 1;
     uint8_t reserved1 : 1;
     uint8_t reserved2 : 1;
     mutable uint8_t ivsel : 1;
     mutable uint8_t ivce : 1;
};
struct ExternalInterruptControlRegisterA
{
     uint8_t reserved : 4;
     mutable uint8_t isc11 : 1;
     mutable uint8_t isc10 : 1;
     mutable uint8_t isc01 : 1;
     mutable uint8_t isc00 : 1;
};
struct PinChangeInterruptControlFlag
{
     uint8_t reserved : 5;
     mutable uint8_t pcif2 : 1;
     mutable uint8_t pcif1 : 1;
     mutable uint8_t pcif0 : 1;
};
struct PinChangeInterruptControlRegister
{
     uint8_t reserved : 5;
     mutable uint8_t pcie2 : 1;
     mutable uint8_t pcie1 : 1;
     mutable uint8_t pcie0 : 1;
};
struct ExternalInterruptFlagRegister
{
     uint8_t reserved : 6;
     mutable uint8_t intf1 : 1;
     mutable uint8_t intf0 : 1;
};
struct ExternalInterruptMaskRegister
{
     uint8_t reserved : 6;
     mutable uint8_t int1 : 1;
     mutable uint8_t int0 : 1;
};

template <bool TMMIO> struct ControlRegisters
{
     uint8_t reserved0[0x1b];
     PinChangeInterruptControlFlag externalPinChangeFlag;
     ExternalInterruptFlagRegister externalIntFlag;
     ExternalInterruptMaskRegister externalIntMask;
     uint8_t reserved1[23];
     MCURegister mcu;
     uint8_t reserved2[42];
};

template <> struct ControlRegisters<true> : ControlRegisters<false>
{
     uint8_t reserved0[8];
     PinChangeInterruptControlRegister intPinChange;
     ExternalInterruptControlRegisterA intControlA;
};

static_assert(sizeof(ControlRegisters<false>) == 0x60);

static_assert(offsetof(ControlRegisters<true>, externalIntFlag) == 0x1c);
static_assert(offsetof(ControlRegisters<true>, externalIntMask) == 0x1d);
static_assert(offsetof(ControlRegisters<true>, mcu) == 0x35);
static_assert(offsetof(ControlRegisters<true>, intControlA) == 0x69);

int main(void)
{
     const volatile ControlRegisters<false>* regs = reinterpret_cast<const volatile ControlRegisters<false>*>(RIOBegin);
     const volatile ControlRegisters<true>* mmio = reinterpret_cast<const volatile ControlRegisters<true>*>(MMIOBegin);
     regs->mcu.ivsel = 1;
}
