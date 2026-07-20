#include "board_pins.h"

int main(void)
{
    ALL_Init();
    while (true)
    {
        speed_control_loop();
    }
}
