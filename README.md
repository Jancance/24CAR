# 24Car MSPM0G3507 第二版扩展板工程

> 更新日期：2026-07-15  
> 当前阶段：第二版扩展板引脚已经进入代码；ICM45686 SPI驱动、姿态解算和OLED验证页面已经完成，并通过Keil全量构建。下一步需要在第二版实物板上验证。

## 当前工程基准

- MCU：MSPM0G3507
- 工具链：VS Code编辑，Keil MDK/ArmClang编译、下载和调试
- 系统时钟：内部SYSOSC经SYSPLL到80MHz
- 当前扩展板：第二版
- 当前主程序：ICM45686通信与姿态解算验证
- 最新构建：`0 Error(s), 0 Warning(s)`
- 自动测试：10项全部通过

第二版完整硬件规划见仓库根目录：

```text
拓展版的引脚复用.md
```

第一版只作为旧PCB排查记录，不再作为代码引脚依据。

## 第二版主要引脚

```text
OLED       PA0 SCL / PA1 SDA

ICM45686   PB9  SPI1 SCLK
            PB8  SPI1 MOSI
            PB14 SPI1 MISO
            PA24 GPIO CS，低电平有效
            PB3  INT1

UART0      PB0 TX / PB1 RX，ZDT1
UART1      PB4 TX / PB5 RX，ZDT2
UART2      PB15 TX / PB16 RX，CanMV K230
UART3      PA26 TX / PA25 RX，USB-TTL调试

DRV8870    PA12 左PWM / PA8 左方向
            PA13 右PWM / PB18右方向

编码器     PA21/PA22，PB19/PB20
灰度       PB6/PB7/PB11/PB12/PB13/PB17/PA17/PB22
按键       PB25/PB26/PB27/PB23
蜂鸣器     PA30
主控RGB    PA14/PA15/PA16
状态LED    PB2/PA31
K230握手   PA28 TRIGGER / PA29 READY
```

第二版已经删除：

```text
MPU6050接口
CCD接口
普通STEP/DIR步进接口
TB6612的PA9/PA7控制用途
```

`PA9`、`PA7` 现在是纯备用GPIO。

## ICM45686代码结构

ICM45686代码集中放在：

```text
project/code/icm45686/
├─ icm45686.c/.h             对外接口、初始化、数据换算
├─ icm45686_port.c/.h        逐飞SPI1、GPIO片选和延时适配
├─ icm45686_attitude.c/.h    静止校准、Mahony六轴融合、四元数和欧拉角
├─ icm45686_page.c/.h        OLED独立验证页面
├─ icm45686_telemetry.c/.h   UART3连续ASCII遥测
└─ vendor/                   ICM45686厂商寄存器驱动
```

参考资料位于：

```text
模块资料/icm45686_SPI
```

没有复制参考工程的SysConfig、时钟、UART、TIMA1和整个TI SDK。当前工程继续使用已经验证的逐飞库和80MHz时钟配置。

## ICM45686工作参数

```text
通信方式     SPI1，四线
SPI模式      Mode 3
初始速率     2MHz
WHO_AM_I     0xE9
加速度量程   ±4g
陀螺仪量程   ±1000dps
输出频率     200Hz
姿态周期     5ms
```

公开接口：

```c
uint8 icm45686_init(void);
uint8 icm45686_update(void);
void icm45686_get_data(icm45686_data_t *data);
void icm45686_get_attitude(icm45686_attitude_t *attitude);
void icm45686_reset_yaw(void);
void icm45686_restart_calibration(void);
```

原始换算结果包括：

```text
accel_g[3]       单位g
gyro_dps[3]      单位度/秒
temperature_c    单位摄氏度
```

姿态结果包括：

```text
yaw / pitch / roll，单位度
quaternion[4]
calibrated，静止零偏校准完成标志
```

## 姿态解算说明

当前使用六轴Mahony风格融合：

- 加速度计用于修正重力方向，稳定Pitch和Roll。
- 陀螺仪用于短期角速度积分。
- 上电后需要保持小车静止约1秒，连续采集200组数据计算三轴陀螺仪零偏。
- 检测到明显运动时，未完成的校准窗口会重新开始。
- 校准完成后OLED显示 `CAL:OK`。
- MODE键松开后调用 `icm45686_reset_yaw()`，把当前车头方向设为相对0度。

ICM45686没有磁力计，因此当前Yaw是相对航向角，不是长期稳定的绝对地理航向。它适合小车短时间直行和90度转向控制，但长时间运行仍会有漂移。后续可结合赛道点位重置、编码器约束或外部视觉结果校正。

当前坐标轴直接使用ICM45686模块本身的XYZ方向。实物安装方向确定后，如果小车前方与传感器X/Y轴不一致，需要在数据进入姿态解算前统一做轴交换和正负号映射。

## 上电后的OLED现象

初始化成功：

```text
ICM45686 OK
Y:  yaw
P:  pitch
R:  roll
GZ: Z轴角速度
CAL:WAIT 或 CAL:OK
```

初始化失败：

```text
ICM45686 ERR
WHO:读取到的芯片ID
CODE:错误码绝对值
```

正常情况下WHO_AM_I应为十进制233，即十六进制`0xE9`。

## UART3连续遥测

UART3通过 `PA26 TX / PA25 RX / GND` 连接USB-TTL，参数为 `115200, 8N1, 无流控`。程序每100ms输出一帧：

```text
$ICM,ms=1250,cal=1,still=1,who=233,ax=0.01,ay=-0.02,az=1.00,gx=0.12,gy=-0.08,gz=0.03,yaw=2.35,pitch=-0.40,roll=0.62,temp=27.10,err=0
```

字段包含时间、校准状态、芯片ID、三轴加速度、三轴角速度、姿态角、温度和错误码。Windows当前识别到USB-TTL为COM4，下载最新固件后可以直接由Codex打开COM4采集，无需手工复制或拍照。

## 实物验证顺序

1. 断电确认ICM45686使用3.3V供电并与MSPM0共地。
2. 确认 `PB9=SCLK、PB8=MOSI、PB14=MISO、PA24=CS、PB3=INT1`。
3. 确认CS有上拉，SPI信号没有接反。
4. 下载当前固件，观察OLED是 `OK` 还是 `ERR`。
5. 上电后保持扩展板静止至少1秒，等待 `CAL:OK`。
6. 静止时观察GZ是否接近0，Pitch/Roll是否稳定。
7. 分别绕模块X、Y、Z轴转动，记录哪个角度变化以及正负方向。
8. 按一下MODE并松开，确认Yaw回到接近0度。
9. 根据模块在小车上的实际朝向确定最终轴映射。

## DRV8870电机接口

第二版扩展板实物电机输出已经确认：

```text
AO1 / AO2 -> 左电机
BO1 / BO2 -> 右电机
```

软件通道对应关系：

```text
左电机：PA12/AIN1 PWM / PA8/AIN2 PWM
右电机：PA13/BIN1 PWM / PB18/BIN2 PWM
```

第二版实物四输入 GPIO 测试确认：

```text
左轮前进 = AIN1 / PA12，左轮后退 = AIN2 / PA8
右轮前进 = BIN2 / PB18，右轮后退 = BIN1 / PA13
```

因此最终方向配置为 `CAR_MOTOR_LEFT_REVERSED=0`、
`CAR_MOTOR_RIGHT_REVERSED=1`。经过该软件反相后，`motor_set()` 的正值应统一表示
车轮驱动车辆向前，负值表示向后。

对外接口保持不变：

```c
motor_init();
motor_set(left, right);
motor_stop();
```

命令范围为 `-10000~10000`。四个DRV8870输入现在全部使用硬件PWM：

- 一个方向：IN1输出PWM，IN2保持低电平。
- 另一个方向：IN1保持低电平，IN2输出PWM。
- 停止：IN1、IN2占空比均为0，进入滑行/低功耗状态。

该方式让正反两个方向的PWM关断阶段都处于`0/0`滑行，消除了旧版“一个方向
驱动/滑行、另一个方向驱动/刹车”造成的启动和调速不对称。

电机上电前仍应先架空车轮，低占空比分别验证左右轮方向。

故障隔离时曾绕过 TIMG0 硬件 PWM，把 `PA12/PA8/PA13/PB18` 全部配置为
普通 GPIO，并使用 1kHz、30% 占空比的软件脉冲分别测试四条输入网络。每条输入
持续约 3 秒，阶段之间停车：

```text
全停 -> LEFT IN1(PA12) -> 停 -> LEFT IN2(PA8) -> 停
     -> RIGHT IN1(PA13) -> 停 -> RIGHT IN2(PB18) -> 全停
```

若四个阶段均能驱动对应电机，说明 PCB 和 DRV8870 输入网络正常，故障位于 TIMG0
硬件 PWM 路径；若 IN1 阶段仍不转而 IN2 能转，则应检查 IN1/BIN1 的 PCB 网络、
10k 电阻接法及焊接。

实测四个输入都能驱动对应电机，说明 DRV8870、四条 PCB 控制网络和 10k 下拉均
基本正常。测试中若电机只抖动一下、OLED长时间停在某阶段，需要优先检查电机启动
造成的 3.3V/5V 电源跌落或主控复位。

第二版实物最终验证结果：

```text
LEFT IN1  -> 左轮顶部朝车头
LEFT IN2  -> 左轮顶部朝车尾
RIGHT IN1 -> 右轮顶部朝车尾
RIGHT IN2 -> 右轮顶部朝车头
```

四个输入现均能稳定驱动对应电机，DRV8870 模块、AO/BO 输出、电机接线和控制输入
验证通过。右轮因镜像安装采用软件反相，保持
`CAR_MOTOR_LEFT_REVERSED=0`、`CAR_MOTOR_RIGHT_REVERSED=1`。

当前主程序已切回正式 `motor_init()/motor_set()` 接口和四路硬件 PWM，以 50%
占空比依次验证左前进、左后退、右前进、右后退及双轮同时前进。该测试用于确认
GPIO 隔离结果已经正确落实到正式电机 API。

20kHz测试时四路输入的万用表平均电压均正确，但电机无动作；1kHz能够正常驱动。
万用表平均电压不能代替示波器对高低电平幅度和电机电流的检查，因此1kHz以上频率
暂不作为已验证配置。

自动测试曾在左轮启动后停留于`LEFT FWD`，因此当前电机测试已经改为TIMG12的
非阻塞毫秒状态机，不再用`system_delay_ms()`等待。PB2绿灯每250ms翻转作为运行
心跳；若电机启动时OLED状态和绿灯心跳同时停止，应检查3.3V电源跌落、电机噪声、
复位和地线，而不是继续修改方向宏。

最终实物复测中，改用非阻塞状态机后OLED阶段、PB2心跳及左右电机正反转均恢复
正常，说明此前停留在`LEFT FWD`的直接原因是阻塞式测试流程没有继续推进，而不是
DRV8870、PCB控制网络或电机损坏。已验证通过的基准组合为：四输入硬件PWM、1kHz、
左右50%测试占空比、左轮不反相、右轮反相。

保持其他条件不变提高到5kHz后，实物表现为电机啸叫但不能起转，因此5kHz测试失败，
当前恢复1kHz。第二版原理图确认R13~R16是四颗独立的10k下拉电阻，不是串联电阻，
也没有与输入组成RC滤波。当前频率差异更可能来自电机绕组电流建立、DRV8870限流
和启动转矩条件；在使用示波器、电流探头进一步检查前，不再提高PWM频率。

## LED与蜂鸣器

- 第二版扩展板实物状态灯颜色已经确认：`PB2` 为绿色 LED，`PA31` 为红色 LED。
- 两颗状态灯均为高电平点亮、低电平熄灭。
- `led_set(0, state)` 控制 `PB2` 绿色状态灯。
- `led_set(1, state)` 控制 `PA31` 红色状态灯。
- `rgb_led_set(0~2, state)` 控制主控板 `PA14/PA15/PA16` RGB，低电平点亮。
- 蜂鸣器已从PA14移动到PA30，代码暂按高电平有效处理。

独立验证程序按 1 秒间隔循环执行：

```text
全部熄灭 -> PB2绿灯点亮 -> PA31红灯点亮 -> 红绿灯同时点亮
```

如果实物PA30蜂鸣器驱动级是低电平有效，只需要修改：

```c
#define CAR_BUZZER_ACTIVE_LEVEL GPIO_LOW
```

## 时钟和SysConfig注意事项

当前 `libraries/sdk/ti_config/ti_msp_dl_config.c/.h` 使用已经验证的内部SYSOSC加PLL 80MHz配置。工程附带的SysConfig与本地DriverLib版本存在历史差异。

在版本统一前，不要直接重新生成并覆盖 `ti_msp_dl_config.c/.h`。ICM45686 SPI通过逐飞 `zf_driver_spi` 初始化，不需要修改或复制参考工程的SysConfig。

## 构建与验证

Keil工程：

```text
project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx
```

## Encoder distance test (2026-07-20)

- Motor encoder: 13 lines per motor revolution; gearbox ratio: 1:28.
- Current decoder counts phase-A rising edges only, so the expected wheel count is `13 * 28 = 364 counts/revolution`.
- Wheel diameter is `65 mm`; circumference is about `204.20 mm`; nominal resolution is about `0.561 mm/count`.
- The current firmware is a passive hand-turn test: motor PWM is not initialized. OLED shows left/right accumulated counts and distance; release the OK key to clear both sides.
- Validation target: turn one wheel exactly one revolution. The OLED should change by about `364 counts` and `204.2 mm`. Forward motion should be positive; if one side is negative, change only that side's encoder reverse macro.
- The TIMG12 1 ms tick, millisecond counter, PIT callback, and `Key_Tick()` call now live in `project/code/timer.c`; `board_pins .c` only calls `system_pit_init()` during full initialization.
- During the first hand-turn test, rotating the left wheel could stall OLED refresh and prevent OK-key clearing, while the right wheel remained smooth. The left PA21 pulse input was therefore moved from per-edge EXTI callbacks to the TIMG6 hardware counter; PA22 remains the direction input. The right encoder remains on PB19 EXTI/PB20 direction for an A/B comparison test.
- TIMG6 pulse+direction mode was rejected after fast rotation caused counts to cancel or stall: an AB encoder's B phase toggles continuously and is not a static direction output. The left encoder now uses true TIMG8 QEI on PA21/PA22. Its x4 hardware count is divided by four with a retained remainder, so the public distance scale remains `364 counts/revolution` without losing slow partial pulses.
- Final x4 test configuration: the left encoder uses TIMG8 hardware QEI; the right encoder uses dual-edge interrupts on both PB19/PB20 plus a quadrature transition table that rejects invalid two-bit jumps. Both public counters now retain x4 resolution: `13 * 28 * 4 = 1456 counts/revolution`, approximately `0.140 mm/count` for the 65 mm wheel.

## Eight-channel grayscale test (2026-07-20)

- Channel order is fixed as array indices `gray_values[0]` through `gray_values[7]`, corresponding to grayscale channels 1 through 8.
- `gray_update()` refreshes the whole array. The normalized value is `1 = black/active`, `0 = white/inactive`, ready for a later weighted-error loop.
- The OLED test refreshes every 50 ms and shows all eight array elements plus raw and active decimal bit masks. Shared board modules are initialized through `ALL_Init()`; the motor initializer leaves all four PWM outputs at zero duty.

## Initialization and include convention

- Application files include only `board_pins.h`; this header is the central include point for board-level module APIs.
- `ALL_Init()` owns the 80 MHz clock setup and initialization of OLED, LEDs, buzzer, keys, motors, encoders, grayscale inputs, the 1 ms system tick, and debug UART.
- `main.c` contains only `ALL_Init()` plus task/test logic. Individual module `.c` files still include their own headers so each module remains independently readable.
- UART3 debug/USB-TTL uses `PA26=TX`, `PA25=RX`, and is configured for `115200, 8N1`.

## Bluetooth transmit/receive test

- The current `main.c` is a UART3 Bluetooth transmit test. Once per second it sends `MSPM0 BT OK, COUNT=n\r\n` at 9600 baud and updates the same send counter on OLED.
- Transmission runs in the main loop using the 1 ms time base; no UART transmission is performed inside an interrupt.
- Open the USB-TTL COM port at `115200, 8N1`. Continuously increasing count values prove the complete MCU-to-PC serial path.
- UART3 RX interrupts only copy bytes into a 64-byte ring buffer. The main loop removes received bytes, updates the OLED RX counter/last-byte display, and echoes each byte to the computer.
- Disable local echo in the PC terminal. Send a short string; receiving the same string back proves the PC-to-MCU and MCU-to-PC paths both work.
- OLED only renders received printable ASCII bytes (`0x20` through `0x7E`). CR/LF and other control bytes are still received and echoed but are not sent to the OLED font renderer; the last printable byte is also shown as a decimal value.
- Second-board hardware feedback confirmed that the PA30 buzzer drive is active-low. `buzzer_init()` now drives PA30 high by default so `ALL_Init()` remains silent.

## PID module

- `project/code/pid.c/.h` provides a reusable position-form PID structure and pointer-based calculation API.
- The implementation stores target, feedback, P/I/D state, integral/output limits, suppresses the first derivative kick, and accepts the real control period in seconds.
- Use a separate `pid_t` instance for every control loop and call `pid_calculate(&instance, target, feedback, dt_s)` from a fixed-period module loop.

## Serial compatibility API

- `project/code/Serial.c/.h` adapts the familiar STM32 tutorial API (`Serial_SendByte/Array/String/Number/Printf` and `Serial_GetRxFlag/Data`) to MSPM0 UART3.
- UART3 initialization, sending, RX interrupt handling and the 64-byte receive ring buffer are all owned by this single module; the duplicate `debug_uart.c/.h` layer has been removed.
- The receive ring buffer preserves bursts of bytes instead of using the tutorial's single-byte overwrite variable.
- `Serial_Printf()` uses bounded `vsnprintf()` rather than unbounded `vsprintf()`. The project already owns its low-level `fputc()` implementation, so this module does not define a conflicting retarget function.

## Dual-wheel speed PI

- `pid.c/.h` is the generic position-form PID calculator. `speed_control.c/.h` owns two independent `pid_t` instances, encoder-speed conversion, filtering, target ramps, motor output, and safety shutdown. `speed_control_config.h` contains all verified wheel-specific parameters.
- The loop runs no faster than every 20 ms, but speed and PID `dt` use the actual elapsed milliseconds. This fixed the false low-speed feedback caused when OLED/UART work stretched a nominal 20 ms cycle.
- Final left PI: `Kp=20`, `Ki=80`, `Kd=0`, integral-output limit 4000 duty, acceleration 500 mm/s^2. Final right PI: `Kp=20`, `Ki=40`, `Kd=0`, integral-output limit 4500 duty, acceleration 1000 mm/s^2. Final motor output is limited to 8000 duty.
- Both encoder directions are inverted in configuration so vehicle-forward feedback is positive. Filtered speed above 1200 mm/s or repeated negative forward feedback stops both motors.
- Integral separation and variable integration are intentionally not part of the generic PID or current speed loop. Independent integral clamps were sufficient after measured load, release, and 600 mm/s tests.
- The final static straight baseline is left 248.5 and right 251.5 mm/s. Its 10-second verification measured 2469 and 2488 mm, a 19 mm or about 0.77% difference. Future long-distance straight control should apply ICM45686 yaw correction around this base instead of retuning the wheel PI.
- The formal speed-loop API now includes `speed_control_set_target_trim(base_mm_s, trim_mm_s)`. Its convention is `left_target=base-trim`, `right_target=base+trim`; therefore a positive trim slows the left wheel and speeds up the right wheel. The yaw outer loop can calculate this trim without changing either wheel's PI parameters.
- The temporary UART/OLED/key harness `dual_speed_test.c/.h` was removed after verification. `main.c` now only initializes the board and repeatedly calls `speed_control_loop()`; power-up remains stopped until a later task module sets targets and calls `speed_control_start()`.

测试命令：

```text
python -m unittest discover -s tests
```

最新结果：

```text
Ran 10 tests
OK

Program Size: Code=35120 RO-data=3876 RW-data=212 ZI-data=5724
0 Error(s), 0 Warning(s)
```
