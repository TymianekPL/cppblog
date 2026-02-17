#include <stddef.h>
#include <stdint.h>

namespace hw
{
     struct Bits8
     {
          uint8_t b0 : 1, b1 : 1, b2 : 1, b3 : 1, b4 : 1, b5 : 1, b6 : 1, b7 : 1;
     };
     struct TccR1A
     {
          uint8_t wgM10 : 1, wgM11 : 1, foC1B : 1, foC1A : 1, coM1B0 : 1, coM1B1 : 1, coM1A0 : 1, coM1A1 : 1;
     };
     struct TccR1B
     {
          uint8_t cS10 : 1, cS11 : 1, cS12 : 1, wgM12 : 1, wgM13 : 1, res : 1, iceS1 : 1, icnC1 : 1;
     };
     struct MaskRegs
     {
          uint8_t toie1 : 1, ocie1a : 1, ocie1b : 1, res : 2, icie1 : 1, res2 : 2;
     };
     struct TimerFlags
     {
          uint8_t tov1 : 1, ocf1a : 1, ocf1b : 1, res : 2, icf1 : 1, res2 : 2;
     };

     struct PortRegs
     {
          volatile uint8_t pin;
          volatile uint8_t ddr;
          volatile uint8_t port;

          template <int NPort>
               requires(NPort >= 0 && NPort < 8)
          void Toggle(void) volatile
          {
               port ^= 1uz << NPort;
          }
          template <int NPort>
               requires(NPort >= 0 && NPort < 8)
          void AsOutput(void) volatile
          {
               ddr |= 1uz << NPort;
          }
     };

     struct Timer1Regs
     {
          union
          {
               volatile uint8_t tccR1A;
               TccR1A a;
          };
          union
          {
               volatile uint8_t tccR1B;
               TccR1B b;
          };
          volatile uint8_t reserved[2];
          volatile uint16_t tcnT1;
          volatile uint8_t reserved2[2];
          volatile uint16_t ocR1A;
     };

     struct TimerMaskReg
     {
          union
          {
               volatile uint8_t raw;

               MaskRegs data;
          };
     };

     struct TimerFlagReg
     {
          union
          {
               volatile uint8_t raw;

               TimerFlags data;
          };
     };

     union MCUCRReg
     {
          volatile uint8_t raw;
          struct
          {
               uint8_t ivce : 1, ivsel : 1, res : 2, pud : 1, bodse : 1, bods : 1, se : 1;
          } data;
     };

     static auto& PortB = *reinterpret_cast<volatile PortRegs*>(0x23);
     static auto& Timer1 = *reinterpret_cast<volatile Timer1Regs*>(0x80);
     static auto& TIMSK1 = *reinterpret_cast<volatile TimerMaskReg*>(0x6F);
     static auto& TIFR1 = *reinterpret_cast<volatile TimerFlagReg*>(0x36);
     static auto& MCUCR = *reinterpret_cast<volatile MCUCRReg*>(0x35);

     static void EnableInterrupts(void) { asm volatile("sei"); }
     static void Sleep(void) { asm volatile("sleep"); }
} // namespace hw

static volatile bool wokeUp = false;

#define DeclareInterrupt(name, number) extern "C" void __vector_##number(void) __attribute__((signal, used))
DeclareInterrupt(TimerISR, 11);
void __vector_11(void) { wokeUp = true; }

static void InitialiseSleep(void)
{
     constexpr uint32_t prescaler = 64;
     constexpr uint32_t ticks = static_cast<uint16_t>(((F_CPU / prescaler / 10) - 1) / 100);

     hw::Timer1.b.wgM12 = 1;
     hw::Timer1.ocR1A = ticks;
     hw::TIMSK1.data.ocie1a = 1;
}

static void GoSleep(void)
{
     wokeUp = false;

     hw::Timer1.tcnT1 = 0;
     hw::TIFR1.data.ocf1a = 1;

     hw::Timer1.b.cS10 = 1;
     hw::Timer1.b.cS11 = 1;

     hw::MCUCR.data.se = 1;

     while (!wokeUp) hw::Sleep();

     hw::Timer1.tccR1B = 0;
}

static void Sleep(size_t ms)
{
     for (size_t i = 0; i < ms; i++) GoSleep();
}

int main(void)
{
     InitialiseSleep();
     hw::EnableInterrupts();
     hw::PortB.AsOutput<5>();

     while (true)
     {
          hw::PortB.Toggle<5>();
          Sleep(500);
     }
}
