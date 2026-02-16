module;

#include <stddef.h>
#include <stdint.h>

#define DeclareInterrupt(number) extern "C" void __vector_##number(void) __attribute__((signal, used))
DeclareInterrupt(11);
#undef DeclareInterrupt

export module Sleep;

export import Hardware;

export namespace sleep
{
     extern volatile bool wokeUp;

     void SetSleepDuration(size_t ms)
     {
          constexpr uint32_t prescaler = 64;
          constexpr uint32_t ticks = (F_CPU / prescaler / 10) - 1;

          hw::PortB.AsOutput<5>();

          hw::Timer1.wgM12 = 1;
          hw::Timer1.ocR1A = static_cast<uint16_t>((ticks * ms) / 100);
          hw::TIMSK1.ocie1a = 1;
     }

     void GoSleep(void)
     {
          sleep::wokeUp = false;

          hw::Timer1.tcnT1 = 0;
          hw::TIFR1.ocf1a = 1;

          hw::Timer1.cS10 = 1;
          hw::Timer1.cS11 = 1;

          hw::MCUCR.se = 1;

          while (!sleep::wokeUp) hw::Sleep();

          hw::Timer1.tccR1B = 0;
     }
} // namespace sleep
