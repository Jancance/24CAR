#ifndef __ICM45686_H
#define __ICM45686_H

#include "icm45686_attitude.h"

#define ICM45686_UPDATE_PERIOD_MS (5U)

uint8 icm45686_init(void);
uint8 icm45686_update(void);
void icm45686_get_data(icm45686_data_t *data);
void icm45686_get_attitude(icm45686_attitude_t *attitude);
void icm45686_reset_yaw(void);
void icm45686_restart_calibration(void);
uint8 icm45686_get_who_am_i(void);
int icm45686_get_last_error(void);

#endif
