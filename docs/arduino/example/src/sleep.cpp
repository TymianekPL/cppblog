module Sleep;

volatile bool sleep::wokeUp = false;

void __vector_11(void) { sleep::wokeUp = true; }
