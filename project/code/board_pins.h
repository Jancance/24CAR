#ifndef __BOARD_PINS_H
#define __BOARD_PINS_H

#include "zf_common_headfile.h"
#include "OLED.h"
#include "Key.h"
#include "buzzer.h"
#include "Serial.h"
#include "encoder.h"
#include "led.h"
#include "motor.h"
#include "gray.h"
#include "timer.h"
#include "pid.h"
#include "speed_control_config.h"
#include "speed_control.h"
#include "bluetooth_link.h"
#include "icm45686/icm45686.h"
#include "position_control.h"

// 第二版扩展板：OLED 使用软件 I2C，SCL/SDA 相对第一版互换。
#define CAR_OLED_SCL_PIN       (A0)
#define CAR_OLED_SDA_PIN       (A1)

// 第二版扩展板：ICM45686 使用 SPI1，CS 由普通 GPIO 手动控制。
#define CAR_ICM45686_SPI       (SPI_1)
#define CAR_ICM45686_SCK_PIN   (SPI1_SCK_B9)
#define CAR_ICM45686_MOSI_PIN  (SPI1_MOSI_B8)
#define CAR_ICM45686_MISO_PIN  (SPI1_MISO_B14)
#define CAR_ICM45686_CS_PIN    (A24)
#define CAR_ICM45686_INT1_PIN  (B3)
#define CAR_ICM45686_SPI_BAUD  (2000000U)

// Debug UART: USB-TTL / Bluetooth / serial screen, use only one at a time.
#define CAR_DEBUG_UART         (UART_3)
#define CAR_DEBUG_UART_TX_PIN  (UART3_TX_A26)
#define CAR_DEBUG_UART_RX_PIN  (UART3_RX_A25)
#define CAR_DEBUG_UART_BAUD    (115200U)

// X42S_V1.0 + Emm_V5.0 TTL: UART0 controls yaw, UART1 controls pitch.
#define CAR_GIMBAL_YAW_UART          (UART_0)
#define CAR_GIMBAL_YAW_UART_TX_PIN   (UART0_TX_B0)
#define CAR_GIMBAL_YAW_UART_RX_PIN   (UART0_RX_B1)
#define CAR_GIMBAL_PITCH_UART        (UART_1)
#define CAR_GIMBAL_PITCH_UART_TX_PIN (UART1_TX_B4)
#define CAR_GIMBAL_PITCH_UART_RX_PIN (UART1_RX_B5)
#define CAR_GIMBAL_UART_BAUD         (115200U)

// CanMV K230：UART2 只传输识别结果，不传输图像。
#define CAR_K230_UART          (UART_2)
#define CAR_K230_UART_TX_PIN   (UART2_TX_B15)
#define CAR_K230_UART_RX_PIN   (UART2_RX_B16)
#define CAR_K230_UART_BAUD     (115200U)
#define CAR_K230_TRIGGER_PIN   (A28)
#define CAR_K230_READY_PIN     (A29)

// 第二版双 DRV8870：四个输入全部使用硬件 PWM，实现对称的驱动/滑行控制。
#define CAR_MOTOR_LEFT_IN1_PWM  (PWM_TIM_G0_CH0_A12)
#define CAR_MOTOR_LEFT_IN2_PWM  (PWM_TIM_A0_CH0_A8)
#define CAR_MOTOR_RIGHT_IN1_PWM (PWM_TIM_G0_CH1_A13)
#define CAR_MOTOR_RIGHT_IN2_PWM (PWM_TIM_A1_CH1_B18)
#define CAR_MOTOR_PWM_FREQ     (20000U)
#define CAR_MOTOR_MAX_DUTY     (PWM_DUTY_MAX)
#define CAR_MOTOR_LEFT_REVERSED  (0U)
#define CAR_MOTOR_RIGHT_REVERSED (1U)

// DC motor encoders: count phase-A rising edges and read phase B for direction.
#define CAR_ENCODER_LEFT_A_PIN   (A21)
#define CAR_ENCODER_LEFT_B_PIN   (A22)
#define CAR_ENCODER_RIGHT_A_PIN  (B19)
#define CAR_ENCODER_RIGHT_B_PIN  (B20)
#define CAR_ENCODER_LINES_PER_MOTOR_REV  (13U)
#define CAR_MOTOR_GEAR_RATIO             (28U)
#define CAR_ENCODER_QUADRATURE_MULTIPLIER (4U)
#define CAR_ENCODER_COUNTS_PER_WHEEL_REV  (CAR_ENCODER_LINES_PER_MOTOR_REV * CAR_MOTOR_GEAR_RATIO * CAR_ENCODER_QUADRATURE_MULTIPLIER)
#define CAR_WHEEL_DIAMETER_MM            (65.0f)
#define CAR_ENCODER_LEFT_REVERSED  (1U)
#define CAR_ENCODER_RIGHT_REVERSED (1U)

// Eight-channel grayscale sensor.
#define CAR_GRAY_1_PIN          (B6)
#define CAR_GRAY_2_PIN          (B7)
#define CAR_GRAY_3_PIN          (B11)
#define CAR_GRAY_4_PIN          (B12)
#define CAR_GRAY_5_PIN          (B13)
#define CAR_GRAY_6_PIN          (B17)
#define CAR_GRAY_7_PIN          (A17)
#define CAR_GRAY_8_PIN          (B22)
#define CAR_GRAY_COUNT          (8U)
#define CAR_GRAY_ACTIVE_LEVEL   (GPIO_LOW)

// 第二版扩展板实物确认：PA30 经驱动级后为低电平有效。
#define CAR_BUZZER_PIN          (A30)
#define CAR_BUZZER_ACTIVE_LEVEL (GPIO_LOW)

// 主控板 RGB 为低电平点亮；第二版扩展板状态 LED 为高电平点亮。
#define CAR_RGB_LED1_PIN        (A14)
#define CAR_RGB_LED2_PIN        (A15)
#define CAR_RGB_LED3_PIN        (A16)
#define CAR_RGB_LED_COUNT       (3U)
#define CAR_STATUS_LED1_PIN     (B2)
#define CAR_STATUS_LED2_PIN     (A31)
#define CAR_STATUS_LED_COUNT    (2U)

// Keys are reserved for later menu and parameter tuning.
#define CAR_KEY_MODE_PIN       (B25)
#define CAR_KEY_PLUS_PIN       (B26)
#define CAR_KEY_MINUS_PIN      (B27)
#define CAR_KEY_OK_PIN         (B23)

// Servo and ultrasonic reserve pins. PA9/PA7 are pure spare GPIOs on V2.
#define CAR_SERVO_PWM          (PWM_TIM_G7_CH1_A27)
#define CAR_ULTRASONIC_TRIG_PIN (B10)
#define CAR_ULTRASONIC_ECHO_PIN (B24)
#define CAR_SPARE_GPIO1_PIN     (A9)
#define CAR_SPARE_GPIO2_PIN     (A7)

void ALL_Init(void);


#endif
