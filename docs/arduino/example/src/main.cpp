#include <stddef.h>
#include <stdint.h>

namespace hw
{
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
          volatile uint8_t pin, ddr, port;

          template <int N>
               requires(N >= 0 && N < 8)
          void Set(void) volatile
          {
               port |= (1u << N);
          }

          template <int N>
               requires(N >= 0 && N < 8)
          void Clear(void) volatile
          {
               port &= ~(1u << N);
          }

          template <int N>
               requires(N >= 0 && N < 8)
          void Toggle(void) volatile
          {
               port ^= 1u << N;
          }

          template <int N>
               requires(N >= 0 && N < 8)
          void AsOutput(void) volatile
          {
               ddr |= (1u << N);
          }
     };

     struct Timer1Regs
     {
          volatile uint8_t tccR1A;
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

     union SMCRReg
     {
          volatile uint8_t raw;
          struct
          {
               uint8_t se : 1, sm0 : 1, sm1 : 1, sm2 : 1, res : 4;
          } data;
     };

     static auto& PortB = *reinterpret_cast<volatile PortRegs*>(0x23);
     static auto& Timer1 = *reinterpret_cast<volatile Timer1Regs*>(0x80);
     static auto& TIMSK1 = *reinterpret_cast<volatile TimerMaskReg*>(0x6F);
     static auto& SMCR = *reinterpret_cast<volatile SMCRReg*>(0x33);

     static void EnableInterrupts(void) { asm volatile("sei"); }
} // namespace hw

static constexpr uint16_t MS_MAX = 1000; // longest single sleep interval

static void SetTimerPeriod(uint16_t ms)
{
     struct
     {
          uint16_t div;
          uint8_t bits;
     } static constexpr prescalers[] = {
         {.div = 1, .bits = 0b001},   {.div = 8, .bits = 0b010},    {.div = 64, .bits = 0b011},
         {.div = 256, .bits = 0b100}, {.div = 1024, .bits = 0b101},
     };

     for (const auto& p : prescalers)
     {
          uint32_t ocr = (F_CPU / (uint32_t)p.div / 1000UL * ms) - 1UL;
          if (ocr <= 0xFFFF)
          {
               hw::Timer1.ocR1A = static_cast<uint16_t>(ocr);
               hw::Timer1.tccR1B = (hw::Timer1.tccR1B & 0b11111000) | p.bits;
               return;
          }
     }
}

struct Context
{
     uint8_t sp[2];
};

__attribute__((naked, noinline)) static void SwitchContext([[maybe_unused]] Context* current,
                                                           [[maybe_unused]] Context* next)
{
     asm volatile("push r2  \n"
                  "push r3  \n"
                  "push r4  \n"
                  "push r5  \n"
                  "push r6  \n"
                  "push r7  \n"
                  "push r8  \n"
                  "push r9  \n"
                  "push r10 \n"
                  "push r11 \n"
                  "push r12 \n"
                  "push r13 \n"
                  "push r14 \n"
                  "push r15 \n"
                  "push r16 \n"
                  "push r17 \n"
                  "push r28 \n"
                  "push r29 \n"

                  "movw r26, r24  \n"
                  "in   r0,  0x3d \n"
                  "st X+, r0 \n"
                  "in   r0,  0x3e \n"
                  "st X+, r0 \n"

                  "movw r26, r22  \n"
                  "ld   r0,  X+   \n"
                  "out 0x3d, r0 \n"
                  "ld   r0,  X+   \n"
                  "out 0x3e, r0 \n"

                  "pop  r29 \n"
                  "pop  r28 \n"
                  "pop  r17 \n"
                  "pop  r16 \n"
                  "pop  r15 \n"
                  "pop  r14 \n"
                  "pop  r13 \n"
                  "pop  r12 \n"
                  "pop  r11 \n"
                  "pop  r10 \n"
                  "pop  r9  \n"
                  "pop  r8  \n"
                  "pop  r7  \n"
                  "pop  r6  \n"
                  "pop  r5  \n"
                  "pop  r4  \n"
                  "pop  r3  \n"
                  "pop  r2  \n"
                  "ret \n" ::
                      : "r0", "r26", "r27", "memory");
}

static constexpr uint8_t MAX_TASKS = 4;
static constexpr size_t STACK_BYTES = 192;

enum class TaskState : uint8_t
{
     Free,
     Ready,
     Sleeping,
     Running
};

struct Task
{
     Context context{};
     uint8_t stack[STACK_BYTES]{};
     uint16_t sleepTicks{}; // ms remaining until wakeup
     TaskState state = TaskState::Free;
     void (*entry)(void) = nullptr;
};

static Task taskStorage[MAX_TASKS]{};
static volatile uint8_t currentTask = 0;
static Context schedulerContext{};

static volatile uint16_t currentPeriodMs = 1;

static volatile bool noop = false;

extern "C" void __vector_11(void) __attribute__((signal, used));
void __vector_11(void)
{
     if (noop) return;

     const uint16_t elapsed = currentPeriodMs;

     for (auto& task : taskStorage)
     {
          if (task.state != TaskState::Sleeping) continue;

          if (task.sleepTicks > elapsed) task.sleepTicks -= elapsed;
          else
          {
               task.sleepTicks = 0;
               task.state = TaskState::Ready;
          }
     }
}

static uint16_t NextWakeupMs(void)
{
     uint16_t best = MS_MAX;
     for (const auto& task : taskStorage)
          if (task.state == TaskState::Sleeping && task.sleepTicks < best)
               best = task.sleepTicks == 0 ? 1 : task.sleepTicks;
     return best;
}

static void TaskSleep(uint16_t ms)
{
     uint8_t idx = currentTask;
     taskStorage[idx].sleepTicks = ms;
     taskStorage[idx].state = TaskState::Sleeping;
     SwitchContext(&taskStorage[idx].context, &schedulerContext);
}

__attribute__((noinline)) static void ThTaskEntry(void)
{
     taskStorage[currentTask].entry();
     taskStorage[currentTask].state = TaskState::Free;
     SwitchContext(&taskStorage[currentTask].context, &schedulerContext);
}

static void SpawnTask(void (*entry)(void))
{
     for (auto& task : taskStorage)
     {
          if (task.state != TaskState::Free) continue;

          task.entry = entry;
          task.sleepTicks = 0;
          task.state = TaskState::Ready;

          uint8_t* top = &task.stack[STACK_BYTES - 1];

          uintptr_t fn = reinterpret_cast<uintptr_t>(ThTaskEntry);
          *top-- = static_cast<uint8_t>(fn & 0xFF);
          *top-- = static_cast<uint8_t>((fn >> 8) & 0xFF);
          for (int i = 0; i < 18; i++) *top-- = 0;

          uintptr_t sp = reinterpret_cast<uintptr_t>(top);
          task.context.sp[0] = static_cast<uint8_t>(sp & 0xFF);
          task.context.sp[1] = static_cast<uint8_t>((sp >> 8) & 0xFF);
          return;
     }
}

[[noreturn]] static void SchedulerLoop(void)
{
     while (true)
     {
          bool taskRan = false;
          bool anyoneWaiting = false;

          for (uint8_t i = 0; i < MAX_TASKS; i++)
          {
               if (taskStorage[i].state == TaskState::Ready)
               {
                    currentTask = i;
                    taskStorage[i].state = TaskState::Running;
                    SwitchContext(&schedulerContext, &taskStorage[i].context);
                    taskRan = true;
               }
               if (taskStorage[i].state == TaskState::Sleeping) anyoneWaiting = true;
          }

          if (!taskRan && anyoneWaiting)
          {
               asm volatile("cli");
               uint16_t nextMs = NextWakeupMs();
               currentPeriodMs = nextMs;
               SetTimerPeriod(nextMs);
               asm volatile("sei \n sleep" ::: "memory");
          }
     }
}

static void InitialiseTimer(void)
{
     hw::Timer1.tccR1A = 0;
     hw::Timer1.tccR1B = 0;
     hw::Timer1.b.wgM12 = 1;
     hw::Timer1.ocR1A = static_cast<uint16_t>((F_CPU / 64UL / 1000UL) - 1UL);
     hw::Timer1.tcnT1 = 0;
     hw::TIMSK1.data.ocie1a = 1;

     hw::Timer1.b.cS10 = 1; // prescaler /64
     hw::Timer1.b.cS11 = 1;

     hw::SMCR.data.sm2 = 1; // standby sleep mode (110)
     hw::SMCR.data.sm1 = 1;
     hw::SMCR.data.sm0 = 0;
     hw::SMCR.data.se = 1;
}

void Thread1(void)
{
     while (true)
     {
          hw::PortB.Set<5>();
          TaskSleep(500);
          hw::PortB.Clear<5>();
          TaskSleep(500);
     }
}

void Thread2(void)
{
     while (true)
     {
          hw::PortB.Toggle<5>();
          TaskSleep(400);
     }
}

int main(void)
{
     InitialiseTimer();
     hw::EnableInterrupts();
     hw::PortB.AsOutput<5>();

     SpawnTask(Thread1);
     SpawnTask(Thread2);

     SchedulerLoop();
}
