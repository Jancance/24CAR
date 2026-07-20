#include "zdt.h"

#define ZDT_RX_BUFFER_SIZE (16U)

typedef struct
{
    uint8 data[ZDT_RX_BUFFER_SIZE];
    uint8 count;
    uint8 expected_length;
} zdt_rx_t;

static zdt_rx_t zdt_rx[ZDT_PORT_COUNT];
static zdt_state_t zdt_state[ZDT_PORT_COUNT];

static uint16 zdt_limit_rpm(uint16 rpm)
{
    return (rpm > ZDT_MAX_RPM) ? ZDT_MAX_RPM : rpm;
}

static uint32 zdt_magnitude(int32 value)
{
    if (value < 0)
    {
        return (uint32)(-(value + 1)) + 1U;
    }
    return (uint32)value;
}

static void zdt_send_position(zdt_port_enum port,
                              uint8 addr,
                              int32 pulse,
                              uint16 rpm,
                              uint8 acceleration,
                              zdt_position_mode_enum mode,
                              uint8 synchronous)
{
    uint32 magnitude = zdt_magnitude(pulse);
    uint8 frame[13];

    frame[0] = addr;
    frame[1] = 0xFDU;
    frame[2] = (pulse < 0) ? (uint8)ZDT_DIR_CCW : (uint8)ZDT_DIR_CW;
    rpm = zdt_limit_rpm(rpm);
    frame[3] = (uint8)(rpm >> 8);
    frame[4] = (uint8)rpm;
    frame[5] = acceleration;
    frame[6] = (uint8)(magnitude >> 24);
    frame[7] = (uint8)(magnitude >> 16);
    frame[8] = (uint8)(magnitude >> 8);
    frame[9] = (uint8)magnitude;
    frame[10] = (uint8)mode;
    frame[11] = synchronous ? 1U : 0U;
    frame[12] = ZDT_CHECK_BYTE;
    zdt_port_write(port, frame, sizeof(frame));
}

static uint8 zdt_expected_length(uint8 function)
{
    if (function == 0x36U)
    {
        return 8U;
    }
    if (function == 0x3AU)
    {
        return 4U;
    }

    // Emm_V5.0 control acknowledgements are address/function/status/check.
    return 4U;
}

static void zdt_parse_frame(zdt_port_enum port, const uint8 *frame, uint8 length)
{
    uint32 magnitude;

    if ((length < 4U) || (frame[length - 1U] != ZDT_CHECK_BYTE))
    {
        zdt_state[port].invalid_frame_count++;
        return;
    }

    if ((frame[1] == 0x36U) && (length == 8U))
    {
        magnitude = ((uint32)frame[3] << 24)
                  | ((uint32)frame[4] << 16)
                  | ((uint32)frame[5] << 8)
                  | (uint32)frame[6];
        zdt_state[port].position_count = (frame[2] != 0U)
                                       ? -(int32)magnitude
                                       : (int32)magnitude;
        zdt_state[port].position_valid = 1U;
    }
    else if ((frame[1] == 0x3AU) && (length == 4U))
    {
        zdt_state[port].status_flags = frame[2];
        zdt_state[port].status_valid = 1U;
    }
    else
    {
        zdt_state[port].last_command_status = frame[2];
    }

    zdt_state[port].valid_frame_count++;
}

void zdt_init(zdt_port_enum port)
{
    if (port >= ZDT_PORT_COUNT)
    {
        return;
    }

    zdt_rx[port].count = 0U;
    zdt_rx[port].expected_length = 0U;
    zdt_state[port].position_count = 0;
    zdt_state[port].position_valid = 0U;
    zdt_state[port].status_flags = 0U;
    zdt_state[port].status_valid = 0U;
    zdt_state[port].last_command_status = 0U;
    zdt_state[port].valid_frame_count = 0U;
    zdt_state[port].invalid_frame_count = 0U;
    zdt_port_init(port);
}

void zdt_enable(zdt_port_enum port, uint8 addr, uint8 synchronous)
{
    uint8 frame[6] = {addr, 0xF3U, 0xABU, 0x01U, synchronous ? 1U : 0U, ZDT_CHECK_BYTE};
    zdt_port_write(port, frame, sizeof(frame));
}

void zdt_disable(zdt_port_enum port, uint8 addr, uint8 synchronous)
{
    uint8 frame[6] = {addr, 0xF3U, 0xABU, 0x00U, synchronous ? 1U : 0U, ZDT_CHECK_BYTE};
    zdt_port_write(port, frame, sizeof(frame));
}

void zdt_move_absolute(zdt_port_enum port,
                       uint8 addr,
                       int32 pulse,
                       uint16 rpm,
                       uint8 acceleration,
                       uint8 synchronous)
{
    zdt_send_position(port, addr, pulse, rpm, acceleration,
                      ZDT_POS_ABSOLUTE, synchronous);
}

void zdt_move_relative(zdt_port_enum port,
                       uint8 addr,
                       int32 pulse,
                       uint16 rpm,
                       uint8 acceleration,
                       uint8 synchronous)
{
    zdt_send_position(port, addr, pulse, rpm, acceleration,
                      ZDT_POS_RELATIVE_TO_CURRENT, synchronous);
}

void zdt_stop(zdt_port_enum port, uint8 addr, uint8 synchronous)
{
    uint8 frame[5] = {addr, 0xFEU, 0x98U, synchronous ? 1U : 0U, ZDT_CHECK_BYTE};
    zdt_port_write(port, frame, sizeof(frame));
}

void zdt_synchronous_start(zdt_port_enum port, uint8 addr)
{
    uint8 frame[4] = {addr, 0xFFU, 0x66U, ZDT_CHECK_BYTE};
    zdt_port_write(port, frame, sizeof(frame));
}

void zdt_set_current_zero(zdt_port_enum port, uint8 addr)
{
    uint8 frame[4] = {addr, 0x0AU, 0x6DU, ZDT_CHECK_BYTE};
    zdt_port_write(port, frame, sizeof(frame));
}

void zdt_read_position(zdt_port_enum port, uint8 addr)
{
    uint8 frame[3] = {addr, 0x36U, ZDT_CHECK_BYTE};
    zdt_port_write(port, frame, sizeof(frame));
}

void zdt_read_status(zdt_port_enum port, uint8 addr)
{
    uint8 frame[3] = {addr, 0x3AU, ZDT_CHECK_BYTE};
    zdt_port_write(port, frame, sizeof(frame));
}

void zdt_update(zdt_port_enum port)
{
    uint8 byte;
    zdt_rx_t *rx;

    if (port >= ZDT_PORT_COUNT)
    {
        return;
    }

    rx = &zdt_rx[port];
    while (zdt_port_read(port, &byte))
    {
        if (rx->count >= ZDT_RX_BUFFER_SIZE)
        {
            rx->count = 0U;
            rx->expected_length = 0U;
            zdt_state[port].invalid_frame_count++;
        }

        rx->data[rx->count++] = byte;
        if (rx->count == 2U)
        {
            rx->expected_length = zdt_expected_length(rx->data[1]);
        }

        if ((rx->expected_length != 0U) && (rx->count == rx->expected_length))
        {
            zdt_parse_frame(port, rx->data, rx->count);
            rx->count = 0U;
            rx->expected_length = 0U;
        }
    }
}

void zdt_get_state(zdt_port_enum port, zdt_state_t *state)
{
    if ((state == NULL) || (port >= ZDT_PORT_COUNT))
    {
        return;
    }
    *state = zdt_state[port];
}
