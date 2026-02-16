module;

#include <stddef.h>
#include <stdint.h>

export module Hardware;

export namespace hw
{
     struct Bits8
     {
          uint8_t b0 : 1, b1 : 1, b2 : 1, b3 : 1, b4 : 1, b5 : 1, b6 : 1, b7 : 1;
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
               struct
               {
                    uint8_t wgM10 : 1, wgM11 : 1, foC1B : 1, foC1A : 1, coM1B0 : 1, coM1B1 : 1, coM1A0 : 1, coM1A1 : 1;
               };
          };
          union
          {
               volatile uint8_t tccR1B;
               struct
               {
                    uint8_t cS10 : 1, cS11 : 1, cS12 : 1, wgM12 : 1, wgM13 : 1, res : 1, iceS1 : 1, icnC1 : 1;
               };
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
               struct
               {
                    uint8_t toie1 : 1, ocie1a : 1, ocie1b : 1, res : 2, icie1 : 1, res2 : 2;
               };
          };
     };

     struct TimerFlagReg
     {
          union
          {
               volatile uint8_t raw;
               struct
               {
                    uint8_t tov1 : 1, ocf1a : 1, ocf1b : 1, res : 2, icf1 : 1, res2 : 2;
               };
          };
     };

     union MCUCRReg
     {
          volatile uint8_t raw;
          struct
          {
               uint8_t ivce : 1, ivsel : 1, res : 2, pud : 1, bodse : 1, bods : 1, se : 1;
          };
     };

     auto& PortB = *reinterpret_cast<volatile PortRegs*>(0x23);
     auto& Timer1 = *reinterpret_cast<volatile Timer1Regs*>(0x80);
     auto& TIMSK1 = *reinterpret_cast<volatile TimerMaskReg*>(0x6F);
     auto& TIFR1 = *reinterpret_cast<volatile TimerFlagReg*>(0x36);
     auto& MCUCR = *reinterpret_cast<volatile MCUCRReg*>(0x35);

     void EnableInterrupts(void) { asm volatile("sei"); }
     void Sleep(void) { asm volatile("sleep"); }
} // namespace hw
