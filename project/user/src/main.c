#include "board_pins.h"

int main(void)
{
    ALL_Init();
    speed_tune_init();
    while (true)
    {
        speed_control_loop();
        speed_tune_loop();
    }
}
