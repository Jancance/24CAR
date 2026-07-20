#include "timer.h"
#include "Key.h"

static volatile uint32 system_ms;

static void system_pit_handler(uint32 state, void *ptr)
{
    (void)state;
    (void)ptr;
    system_ms++;
    Key_Tick();
}

void system_pit_init(void)
{
    system_ms = 0U;
    pit_ms_init(CAR_SYSTEM_PIT, 1U, system_pit_handler, NULL);
    interrupt_set_priority(CAR_SYSTEM_PIT_IRQn, 0U);
}

uint32 system_get_ms(void)
{
    return system_ms;
}
