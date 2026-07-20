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

对外接口保持不变：

```c
motor_init();
motor_set(left, right);
motor_stop();
```

命令范围为 `-10000~10000`。内部已经从TB6612双方向输入改成DRV8870的PWM加方向输入：

- 正转：方向脚低，普通PWM。
- 反转：方向脚高，反相PWM。
- 停止：PWM和方向脚都拉低，进入滑行/低功耗状态。

电机上电前仍应先架空车轮，低占空比分别验证左右轮方向。

## LED与蜂鸣器

- `led_set(0/1, state)` 控制第二版 `PB2/PA31` 状态LED，高电平点亮。
- `rgb_led_set(0~2, state)` 控制主控板 `PA14/PA15/PA16` RGB，低电平点亮。
- 蜂鸣器已从PA14移动到PA30，代码暂按高电平有效处理。

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
