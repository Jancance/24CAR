#ifndef __ZDT_H
#define __ZDT_H

#include "zdt_port.h"

#define ZDT_DEFAULT_ADDR                 (1U)
#define ZDT_CHECK_BYTE                   (0x6BU)
#define ZDT_MAX_RPM                      (5000U)
#define ZDT_POSITION_FEEDBACK_PER_REV    (65536.0f)

typedef enum
{
    ZDT_DIR_CW = 0,
    ZDT_DIR_CCW = 1
} zdt_dir_enum;

typedef enum
{
    ZDT_POS_RELATIVE_TO_TARGET = 0,
    ZDT_POS_ABSOLUTE = 1,
    ZDT_POS_RELATIVE_TO_CURRENT = 2
} zdt_position_mode_enum;

typedef struct
{
    int32 position_count;       // Emm_V5.0 0x36 feedback: 65536 counts per motor revolution.
    uint8 position_valid;
    uint8 status_flags;         // Raw 0x3A status byte; keep raw to match future firmware revisions.
    uint8 status_valid;
    uint8 last_command_status;  // Raw command acknowledgement status byte.
    uint32 valid_frame_count;
    uint32 invalid_frame_count;
} zdt_state_t;

void zdt_init(zdt_port_enum port);
void zdt_enable(zdt_port_enum port, uint8 addr, uint8 synchronous);
void zdt_disable(zdt_port_enum port, uint8 addr, uint8 synchronous);
void zdt_move_absolute(zdt_port_enum port,
                       uint8 addr,
                       int32 pulse,
                       uint16 rpm,
                       uint8 acceleration,
                       uint8 synchronous);
void zdt_move_relative(zdt_port_enum port,
                       uint8 addr,
                       int32 pulse,
                       uint16 rpm,
                       uint8 acceleration,
                       uint8 synchronous);
void zdt_stop(zdt_port_enum port, uint8 addr, uint8 synchronous);
void zdt_synchronous_start(zdt_port_enum port, uint8 addr);
void zdt_set_current_zero(zdt_port_enum port, uint8 addr);
void zdt_read_position(zdt_port_enum port, uint8 addr);
void zdt_read_status(zdt_port_enum port, uint8 addr);

// Call regularly from the main loop. It consumes received UART bytes without blocking.
void zdt_update(zdt_port_enum port);
void zdt_get_state(zdt_port_enum port, zdt_state_t *state);

#endif
