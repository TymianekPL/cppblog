import Sleep;

int main(void)
{
     sleep::SetSleepDuration(100);
     hw::EnableInterrupts();

     while (true)
     {
          hw::PortB.Toggle<5>();
          sleep::GoSleep();
     }
}
