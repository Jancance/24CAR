#include "board_pins.h"

int main(void)
{
    ALL_Init();
    task_manager_init();
    bluetooth_link_init();
    while (true)
    {
        speed_control_loop();
        line_follow_loop();
        key_Loop();
        if (!line_follow_is_active() || task_manager_is_running())
        {
            position_control_loop();
        }
        task_manager_display_loop();
        bluetooth_link_loop();
        buzzer_loop();
    }
}
