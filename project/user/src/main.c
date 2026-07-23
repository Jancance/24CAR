#include "board_pins.h"

int main(void)
{
    ALL_Init();
    bluetooth_link_init();
    while (true)
    {
        speed_control_loop();
        position_control_loop();
        bluetooth_link_loop();
    }
}
