# 24Car 工程协作说明

## 工程范围

- 实际 Git 仓库根目录是本目录：`SeekFree_MSPM0G3507_Opensource_Library`，不是外层 `C:\Users\lenovo\Desktop\24Car`。
- MCU：MSPM0G3507。
- 开发方式保持为：VS Code 编辑，Keil MDK/ArmClang 编译、下载和调试。
- Keil 工程：`project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx`。
- 当前硬件是第二版拓展板；第一版资料只能用于历史排查，不能覆盖第二版实物结论。
- 不要随意重新生成 `libraries/sdk/ti_config/ti_msp_dl_config.c/.h`。当前内部 SYSOSC 经 SYSPLL 到 80 MHz 的配置已经验证，SysConfig 与本地 DriverLib 存在版本差异。

## 代码组织约定

- `project/code/board_pins.h` 是应用层统一头文件入口，集中引用板级模块头文件和保存第二版拓展板引脚定义。
- `project/user/src/main.c` 原则上只写 `#include "board_pins.h"`，不要重复包含各模块头文件。
- 所有板级模块初始化集中放入 `ALL_Init()`；主程序只调用一次 `ALL_Init()`，随后进入任务或测试逻辑。
- `ALL_Init()` 当前负责：80 MHz 系统时钟、OLED、LED、蜂鸣器、按键、电机、编码器、8 路灰度、1 ms 系统时基和调试串口。
- 各模块自己的 `.c` 文件仍应引用自己的 `.h`，保持模块独立、接口清晰。
- 新功能优先拆成可复用的 `.c/.h` 模块，不要把驱动细节堆进 `main.c`。
- 硬件适配和极性统一通过 `board_pins.h` 宏或模块封装处理，上层任务代码使用统一语义。

## 已验证硬件结论

### OLED

- 软件 I2C：`PA0=SCL`，`PA1=SDA`。
- OLED 已在第二版拓展板实物验证通过。

### ICM45686

- SPI1：`PB9=SCLK`、`PB8=MOSI`、`PB14=MISO`、`PA24=CS`、`PB3=INT1`。
- SPI 波特率当前为 2 MHz。
- Yaw 读取已验证；上电或复位完成校准后以当前朝向清零。

### 调试串口与蓝牙

- UART3：`PA26=TX`、`PA25=RX`。
- 当前调试/USB-TTL串口参数为 `115200, 8N1`。电脑端串口工具也必须设置为115200。
- 若以后修改波特率，必须同时修改MSPM0、主机蓝牙、从机蓝牙和电脑串口工具，不能只改其中一端。
- 串口统一使用 `Serial.c/.h`：该模块直接负责MSPM0 UART3初始化、发送、接收中断和64字节环形缓冲，不再保留重复的 `debug_uart.c/.h`。禁止把STM32的RCC/GPIO/USART寄存器代码或 `USART1_IRQHandler()`直接复制进本工程。

### DRV8870 与电机

- R11 已从 12kΩ 改为 4.7kΩ、1%，R12 保持 10kΩ；VREF 实测 2.23V，配合 0.1Ω 采样电阻实际限流约 2.23A。20kHz、30% 占空比架空复测正转左360/右365mm/s、反转左359/右363mm/s，均连续、无异响且模块温度正常。
- `AO1/AO2` 驱动左电机，`BO1/BO2` 驱动右电机。
- 四路控制均为硬件 PWM：
  - 左 IN1：`PA12/TIMG0_C0`
  - 左 IN2：`PA8/TIMA0_C0`
  - 右 IN1：`PA13/TIMG0_C1`
  - 右 IN2：`PB18/TIMA1_C1`
- 实物方向：
  - 左轮前进：PA12/IN1 固定 100%，PA8/IN2 输出反相 PWM。
  - 左轮后退：PA8/IN2 固定 100%，PA12/IN1 输出反相 PWM。
  - 右轮前进：PB18/IN2 固定 100%，PA13/IN1 输出反相 PWM。
  - 右轮后退：PA13/IN1 固定 100%，PB18/IN2 输出反相 PWM。
- 方向宏保持：左轮不反相，右轮反相，即 `CAR_MOTOR_LEFT_REVERSED=0`、`CAR_MOTOR_RIGHT_REVERSED=1`。
- `motor_set()` 的正值统一表示车辆前进方向，负值表示后退。
- 当前采用 20kHz 驱动/制动慢衰减。30% 占空比架空实测：10kHz 左373/右369mm/s，20kHz 左370/右368mm/s；均持续转动、无异响、运行平滑且驱动模块无明显温升。旧驱动/滑行模式的 5/20kHz 失败只作为历史记录，不得据此把频率恢复到1kHz。
- 左轮速度环台架参数为 `Kp=20, Ki=80, Kd=0`，控制周期20 ms；已验证100/200/300 mm/s和持续人工加载。Ki=40负载恢复偏慢，Ki=80可恢复到约195～204 mm/s，松载短时峰值约222 mm/s且无持续振荡。左编码器前进方向需配置 `CAR_ENCODER_LEFT_REVERSED=1`。
- `speed_control.c/.h` 保留为正式双轮速度闭环：20 ms为最小调度间隔，但编码器速度和PID dt必须使用实际经过时间；上电默认停止，反向反馈或超速会同时停两轮。
- 旧 1kHz 驱动/滑行速度环参数为左 Kp20/Ki80、右 Kp20/Ki40，总输出上限均为8000 PWM，当前左右启动斜坡均为1000mm/s²。切换到20kHz驱动/制动后这些参数只能作为重测初值，不得直接声明为最终值。
- 当前速度环只启用左右轮独立积分限幅；变速积分和积分分离尚未启用。此类策略应根据某个控制环的实测问题在对应业务模块中单独加入，不要塞进通用 `pid.c/.h` 后影响所有 PID 实例。
- 两组10秒地面数据夹住了静态中点：目标247/253得到左2453、右2503 mm并左偏，目标250/250得到左2498、右2453 mm并持续右偏。线性插值后的左248.5、右251.5 mm/s复测得到左2469、右2488 mm，只差19 mm（约0.77%），用户观察仅轻微偏转。该值作为最终静态地面基线，PI和左500/右1000 mm/s²启动斜坡不再继续调整；正式长距离方向控制使用ICM45686 yaw外环。
- 临时串口/OLED/按键速度测试文件 `dual_speed_test.c/.h` 已在调参完成后删除；不要把测试壳重新混入正式控制结构。以后由任务模块设置目标并调用 `speed_control_start/stop/loop`。
- PCB 上 AIN1/AIN2/BIN1/BIN2 的四个 10 kΩ 电阻是独立下拉，不是串联电阻，也没有形成输入 RC 滤波。

### 编码器

- 左轮：`PA21=A`、`PA22=B`。
- 右轮：`PB19=A`、`PB20=B`。
- 电机编码器为 13 线，减速比 1:28，轮胎直径 65 mm。
- 两侧当前都保留四倍频逻辑分辨率：`13 * 28 * 4 = 1456 counts/轮圈`。
- 65 mm 轮胎理论周长约 204.20 mm，理论分辨率约 0.140 mm/count。
- 左轮 PA21/PA22 使用 TIMG8 硬件 QEI 四倍频，避免逐脉冲中断拖住 OLED 和主循环。
- 右轮 PB19/PB20 无法组成同一个硬件 QEI 定时器通道，当前使用 A、B 双边沿中断和正交状态表实现软件四倍频，并过滤非法两位跳变。
- 实物前进测试确认左右编码器原始计数均为负，因此方向宏保持 `CAR_ENCODER_LEFT_REVERSED=1`、`CAR_ENCODER_RIGHT_REVERSED=1`，使车辆前进统一为正速度。
- 不要把 AB 正交编码器直接当成“脉冲+固定方向”编码器。B 相会持续翻转，使用 `encoder_dir_init()` 会造成快速转动时分段计数正负抵消。
- OLED 或控制周期只是读取累计值，不是脉冲采样周期。左侧硬件和右侧中断均持续累计。
- 做速度闭环时建议 10 ms 固定控制周期；OLED 可保持 50～100 ms 刷新，不能让显示刷新决定控制周期。

### 8 路灰度

- 通道顺序固定为：`PB6、PB7、PB11、PB12、PB13、PB17、PA17、PB22`。
- `gray_values[0]` 到 `gray_values[7]` 分别对应灰度通道 1 到 8。
- `gray_update()` 刷新整个数组。
- 统一语义：检测到黑色/有效状态为 1，白色/无效状态为 0；当前有效电平配置为低电平。
- 后续灰度加权必须直接按数组下标循环计算，不要重新散写 8 个独立变量。
- 单通道有效时，active mask 依次应为 `1、2、4、8、16、32、64、128`。

### 按键、LED 与蜂鸣器

- 按键：`PB25=MODE`、`PB26=PLUS`、`PB27=MINUS`、`PB23=OK`；松开事件编号依次为 1、2、3、4。
- 1 ms 中断中调用 `Key_Tick()` 完成按键消抖。
- 第二版拓展板状态灯：`PB2=绿色`、`PA31=红色`，均为高电平点亮。
- 主控板 RGB：`PA14/PA15/PA16`，低电平点亮。
- 蜂鸣器：`PA30`。第二版实物确认低电平有效；初始化必须输出高电平保持静音。

## 1 ms 系统时基

- 1 ms 系统时基实现统一放在 `project/code/timer.c/.h`。
- 当前使用 TIMG12，提供 `system_pit_init()` 和 `system_get_ms()`。
- PIT 回调负责递增毫秒计数并调用 `Key_Tick()`。
- 不要把毫秒变量、PIT 回调或按键节拍重新写回 `board_pins .c` 或 `main.c`。
- 周期任务使用无阻塞时间差判断，避免用长时间 `system_delay_ms()` 阻塞状态机。

## 周期任务与中断调度规范

- 以后需要 2 ms、5 ms、10 ms、20 ms 等周期任务时，统一从 1 ms 系统中断分频，不要为每个模块随意增加阻塞延时。
- 1 ms 中断只能做短时间操作：计数器自增、达到周期后清计数器、置 `volatile` 标志位。禁止在中断中执行 OLED 刷新、串口打印、I2C/SPI 传感器读取、浮点控制计算、电机状态机或任何可能长时间等待的程序。
- 每个有周期任务的模块提供一个 `<module>_Loop()` 函数，并由 `main()` 的无限循环反复调用。
- `<module>_Loop()` 先检查自己的运行标志；没有标志立即返回。发现标志后先以临界区方式清除标志，再在中断外执行实际任务。
- 清标志时要防止与 1 ms 中断竞争。推荐短暂关闭全局中断，仅完成“读取并清零标志”后立即恢复；耗时任务本身必须在开中断状态下运行。
- 如果只关心“至少运行一次”，使用布尔标志；如果任务不能丢失任何周期，使用待处理计数器而不是布尔标志，主循环每执行一次就减一。
- 推荐结构如下：

```c
// module.c
static volatile uint8 module_run_flag;

// 由 1 ms PIT 回调调用；这里只分频和置位。
void module_1ms_tick(void)
{
    static uint16 module_ms;

    module_ms++;
    if (module_ms >= 10U)
    {
        module_ms = 0U;
        module_run_flag = 1U;
    }
}

// 由主循环持续调用；耗时工作全部放在这里。
void module_Loop(void)
{
    uint32 primask;

    if (!module_run_flag)
    {
        return;
    }

    primask = interrupt_global_disable();
    module_run_flag = 0U;
    interrupt_global_enable(primask);

    // 在这里读取传感器、计算控制量、刷新状态等。
}
```

- 主循环保持非阻塞并统一调度：

```c
while (true)
{
    gray_Loop();
    encoder_Loop();
    motor_Loop();
    task_Loop();
}
```

- 某个 `_Loop()` 不得长期占用主循环；较长流程继续拆成状态机，单次调用只推进一步。

## 当前程序状态

- `main.c` 当前运行临时 `drv8870_decay_test`：20kHz、30% 驱动/制动，MODE切换方向、OK启停、5秒自动停车，OLED显示最后0.5秒左右轮平均速度。
- `ALL_Init()` 初始化速度控制器，但上电不设置运行状态，四路PWM保持为0，因此不会自行驱动车轮。
- UART3/Serial通用模块仍保留，但当前实物测试不依赖串口。VREF 与正反转复测已完成，临时开环测试已删除，当前入口为速度环测试；OLED 在5秒测试结束后显示去掉前2秒启动段的左右平均速度和峰值PWM。

## PID 模块约定

- 通用位置式 PID 位于 `project/code/pid.c/.h`，通过 `pid_t *` 传递实例，速度环、方向环等必须创建各自独立的结构体。
- 速度环提供 `speed_control_set_target_trim(base, trim)`：正trim约定为左目标减小、右目标增大，供后续ICM45686 yaw外环使用；差速修正不应塞进通用PID。
- 关系为：`error=target-feedback`、`integral+=error*dt`、`derivative=(error-previous_error)/dt`、`output=kp*error+ki*integral+kd*derivative`。
- `dt_s` 的单位是秒；10 ms 控制周期传入 `0.010f`。
- PID计算只能在固定周期的模块 `_Loop()` 中执行，不得放在1 ms中断里。
- 每个具体PID实例的 `pid_init()` 应在确定参数后加入 `ALL_Init()`或对应的统一任务初始化入口；通用PID模块本身不创建隐含的全局控制器。

## 构建与验证

- 从外层目录 `C:\Users\lenovo\Desktop\24Car` 运行契约测试：

```powershell
python -m unittest discover -s tests
```

- Keil 全量构建命令：

```powershell
& 'C:\Users\lenovo\AppData\Local\Keil_v5\UV4\UV4.exe' -r 'C:\Users\lenovo\Desktop\24Car\SeekFree_MSPM0G3507_Opensource_Library\project\mdk\SeekFree_MSPM0G3507_Device_Library.uvprojx' -j0
```

- 权威构建结果读取：`project/mdk/Objects/SeekFree_MSPM0G3507_Device_Library.build_log.htm`。
- 最近一次统一初始化结构验证结果：21 项测试通过，Keil `0 Error(s), 0 Warning(s)`。
- 嵌入式功能不能只以“编译通过”为完成标准；还要结合 OLED/UART 状态、引脚电压和实物运动结果验证。

## Git 与改动纪律

- GitHub 远程：`https://github.com/Jancance/24CAR.git`，默认分支 `main`。
- 已推送的拓展板模块验证基线提交：`923b66d feat: verify expansion board modules`。
- 提交前检查真实仓库的 `git status` 和 `git diff --check`，不要从外层非仓库目录操作。
- 保留用户已有改动，不要使用 `git reset --hard` 或覆盖无关文件。
- 当前部分历史中文注释存在编码显示问题；除非用户明确要求，不要进行全仓编码转换或大范围格式化。
