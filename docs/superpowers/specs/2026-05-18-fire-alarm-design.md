# 声光警报器固件设计规格

## 概述

火灾报警领域的声光警报器固件。基于华大 HC32L110C6PA（Cortex-M0+），裸机运行，后续可移植到 ST/瑞萨平台。

## 功能需求

1. **声警报** — 无源蜂鸣器 PWM 驱动，支持多种 pattern（频率+占空比+时长组合）
2. **光警报** — LED PWM 驱动，支持多种 pattern
3. **Pattern 选择** — 两组独立的 2-bit DIP 开关，分别选择声 pattern 和光 pattern
4. **测试通信** — UART 文本命令协议，上位机可控制/查询/自检
5. **私有协议通信** — 预留，暂不实现

## 开发环境

- **工具链**: VSCode + GCC ARM + CMake
- **调试器**: J-Link / ST-Link + Cortex-Debug 插件
- **上位机连接**: 外接 CH340/CP2102 USB-UART 模块（PCB 预留排针）

## 软件架构

```
┌─────────────────────────────┐
│   App 层（main loop）        │
│   - 声光 pattern 调度        │
│   - DIP 开关读取 & 选择      │
│   - 测试命令解析 & 执行       │
├─────────────────────────────┤
│   Driver API（接口头文件）    │
│   pwm_drv.h / gpio_drv.h    │
│   uart_drv.h / timer_drv.h  │
├─────────────────────────────┤
│   HAL 层（per MCU）          │
│   hc32l110/*.c → 未来替换   │
│   为 stm32/*.c / renesas/*.c │
└─────────────────────────────┘
```

## 目录结构

```
fire-alarm/
├── CMakeLists.txt
├── toolchain/
│   └── arm-gcc.cmake
├── src/
│   ├── main.c
│   ├── app/
│   │   ├── alarm_sound.c/h
│   │   ├── alarm_light.c/h
│   │   └── test_protocol.c/h
│   ├── driver/
│   │   ├── pwm_drv.h
│   │   ├── gpio_drv.h
│   │   ├── uart_drv.h
│   │   └── timer_drv.h
│   ├── hal/
│   │   └── hc32l110/
│   │       ├── pwm_drv.c
│   │       ├── gpio_drv.c
│   │       ├── uart_drv.c
│   │       └── timer_drv.c
│   └── config/
│       └── pattern_table.c/h
├── test/
│   └── test_protocol_spec.md
└── docs/
    └── architecture.md
```

## Driver API

### PWM 驱动

```c
void pwm_init(uint8_t channel, uint32_t freq_hz, uint8_t duty_pct);
void pwm_set_freq(uint8_t channel, uint32_t freq_hz);
void pwm_set_duty(uint8_t channel, uint8_t duty_pct);
void pwm_start(uint8_t channel);
void pwm_stop(uint8_t channel);
```

channel 0 = 蜂鸣器, channel 1 = LED。声光可同时独立工作。

### GPIO 驱动

```c
void gpio_init(uint8_t pin, uint8_t mode);
uint8_t gpio_read(uint8_t pin);
void gpio_write(uint8_t pin, uint8_t level);
```

### UART 驱动

```c
void uart_init(uint32_t baudrate);
void uart_send(const uint8_t *buf, uint16_t len);
uint16_t uart_recv(uint8_t *buf, uint16_t max_len);
void uart_send_str(const char *str);
```

### Timer 驱动

```c
void timer_delay_ms(uint32_t ms);
void timer_delay_us(uint32_t us);
uint32_t timer_get_tick(void);
```

所有接口仅依赖 `stdint.h`，不依赖任何 MCU 头文件。channel/pin 使用数字 ID，保证跨平台通用。

**Timer tick 精度**: `timer_get_tick()` 基于 Cortex-M SysTick 实现，tick = 1ms。仅用于 Pattern step 时长调度和消抖计时，不用于 PWM 生成（PWM 频率/占空比由 MCU 硬件 Timer 直接产生，精度为微秒级）。

**Tick 溢出处理**: 1ms tick 的 uint32_t 约 49.7 天溢出。所有时长比较必须用无符号差值 `(now - start) >= duration`，禁止直接用 `now >= deadline`。

## Pattern 配置表

Pattern 是数据，不是代码。DIP 开关值直接作为表索引。

```c
typedef struct {
    uint32_t freq_hz;
    uint8_t  duty_pct;
    uint16_t duration_ms;
} pattern_step_t;

typedef struct {
    uint8_t         step_count;
    pattern_step_t *steps;
    uint8_t         loop_count;   // 0 = 无限循环，N = 播 N 圈后自动停止
} pattern_t;
```

**有限循环结束行为**: `loop_count > 0` 且播满指定圈数后，自动停止 PWM 输出，蜂鸣器/LED 回到空闲状态。外部通过 DIP 变化或新命令重新触发。

示例（T3 消防模式 3kHz，500ms 周期交替）：

```c
/* T3: 500ms on/off alternating, 3kHz */
pattern_step_t sound_t3_steps[] = {
    {3000, 80, 500},
    {0,    0,  500},
    {3000, 80, 500},
    {0,    0,  500},
    {3000, 80, 500},
    {0,    0, 1500},
};
```

示例（光警报 20ms on / 980ms off，高亮度 = on 期间占空比 100%）：

```c
pattern_step_t light_high_steps[] = {
    {3000, 100,  20},   /* frequency is LED driver compatible */
    {0,    0,   980},
};
```

声光的 pattern 表独立定义，各自管理。**音量/亮度通过占空比区分，复用同一套 step 时序结构**。

**声 DIP 编码**（2-bit, pin 19/20）：

| DIP | Pattern | 波形 | 音量（占空比） |
|-----|---------|------|--------------|
| 00 | sound_p0 | T3 交替 | 小（50%） |
| 01 | sound_p1 | T3 交替 | 大（80%） |
| 10 | sound_p2 | 连续音 | 小（50%） |
| 11 | sound_p3 | 连续音 | 大（80%） |

**光 DIP 编码**（2-bit, pin 2/3）：

| DIP | Pattern | 时序 | 亮度（on期间占空比） |
|-----|---------|------|-------------------|
| 00 | light_p0 | 20ms on / 980ms off | 低（30%） |
| 01 | light_p1 | 20ms on / 980ms off | 中（60%） |
| 10 | light_p2 | 20ms on / 980ms off | 高（100%） |
| 11 | light_p3 | 预留，回退到 light_p0 | — |

**DIP 索引映射**: 声和光各有 4 种组合（0-3）。未使用槽位（index ≥ 4 for future expansion）指向各自的 pattern 0。

**DIP 读取**: 两组独立的 2-bit DIP 开关，分别读取后各得 0-3 的值，查各自的 pattern 表。两组 DIP 互不干扰，声切换不影响光，光切换不影响声。

**声 pattern 0 语义（T3 小音量）**: 上电默认，越界回退时使用。T3 交替音 + 50% 占空比（小音量）。即使不接 DIP 也能正常报警。

**光 pattern 0 语义（低亮度）**: 上电默认，越界回退时使用。20ms on / 980ms off + on 期间 30% 占空比（低亮度）。

**内存预算**: `pattern_step_t` 每个 8 字节。声 4 条 pattern × 各约 6 step + 光 4 条 pattern × 2 step ≈ 256 字节。HC32L110C6PA 有 4KB RAM，充裕。Step 数组加 `const` 放入 Flash（`.rodata`），`pattern_t` 在 RAM。

**DIP 切换行为**: 声 DIP 和光 DIP 独立检测、独立切换。各自检测到变化时立即从新 pattern 的 step 0 重新开始，不等待当前 step 播完。声 DIP 变化不影响光，反之亦然。

**DIP 消抖**: 每路 DIP 独立消抖，各 50ms。连续 50ms 稳定读数才确认变化。

**命令触发切换行为**: `BUZZ ON n` / `LED ON n` 命令与 DIP 切换行为一致——立即从新 pattern 的 step 0 重启。命令完成后 `RESET` 命令恢复为 DIP 定义的 pattern。

## 测试通信协议

UART 115200-8-N-1，文本命令，`\r\n` 分隔。单条命令最大长度 64 字节，超长截断并返回 `ERR CMD`。

| 命令 | 响应 | 说明 |
|------|------|------|
| `BUZZ ON <n>` | `OK BUZZ=<n>` | 启动蜂鸣器 pattern n |
| `BUZZ OFF` | `OK BUZZ=OFF` | 停止蜂鸣器 |
| `LED ON <n>` | `OK LED=<n>` | 启动 LED pattern n |
| `LED OFF` | `OK LED=OFF` | 停止 LED |
| `DIP` | `OK SD=<v> LD=<v>` | 读取声/光拨码开关当前值 |
| `STATUS` | `OK B=<n> L=<n> SD=<v> LD=<v>` | 查询全部状态 |
| `RESET` | `OK RESET` | 恢复到 DIP 定义的 pattern |
| `TEST` | `OK ALARM_TEST` | 自检：声光各闪 3 次，结束后恢复到 DIP 定义的 pattern |

**错误响应**:

| 条件 | 响应 |
|------|------|
| 无法识别的命令 | `ERR CMD` |
| pattern 索引超出合法范围 | `ERR RANGE` |
| 执行失败（硬件异常） | `ERR FAULT` |

解析通过命令表驱动，新增命令加一行表项即可。后续换二进制协议只需替换编解码层。

## Super Loop 主循环

```c
while (1) {
    uint32_t now = timer_get_tick();

    // 1. 声警报状态机
    if (alarm_sound_is_active()) alarm_sound_tick(now);

    // 2. 光警报状态机
    if (alarm_light_is_active()) alarm_light_tick(now);

    // 3. DIP 开关变化检测（两组独立）
    uint8_t sound_dip = sound_dip_debounced_read();
    if (sound_dip != cur_sound_dip) {
        cur_sound_dip = sound_dip;
        alarm_sound_load(cur_sound_dip);
    }
    uint8_t light_dip = light_dip_debounced_read();
    if (light_dip != cur_light_dip) {
        cur_light_dip = light_dip;
        alarm_light_load(cur_light_dip);
    }

    // 4. UART 命令处理
    if (uart_recv(rx_buf, len) > 0) test_protocol_feed(rx_buf);
}
```

4 个模块互不阻塞，声光各自独立的状态机。

**上电行为**: `alarm_sound_load()` / `alarm_light_load()` 语义为"加载 pattern 并立即从 step 0 开始播放"。上电后自动以 DIP 选定的 pattern 开始报警，无需额外触发。`load` + `start` 为原子操作。

**边界处理**: 声/光 DIP 读取各自做上限钳位（>3 → 0）。`alarm_sound_load()` / `alarm_light_load()` 内部检查索引合法性，越界回退到各自的 pattern 0。

## 移植策略

移植到新 MCU 时：
1. 新增 `hal/<platform>/` 目录，实现 4 个 driver 的 `.c` 文件
2. 修改 `CMakeLists.txt` 中的 HAL 源文件路径
3. 调整 `toolchain/` 下的 linker script 和启动文件
4. `app/` 和 `driver/*.h` 不动

## 不包含在本期

- 私有协议通信（预留接口，后续实现）
- 上位机测试脚本（预留 `test/` 目录，依赖外接 USB-UART 模块）
- 低功耗功能
- Bootloader / OTA

## 硬件依赖

- MCU: 华大 HC32L110C6PA (Cortex-M0+, 32KB Flash, 4KB RAM, TSSOP-20)
- 外部晶振: 8MHz (pin 5/6)
- 无源压电蜂鸣器 + 三极管驱动电路（MCU PWM → 开漏输出）
- LED + LED 电源专用芯片驱动（MCU PWM → 芯片输入端，输出波形跟随）
- 声 DIP 开关: 2-bit (pin 19/20)，公共端 GND，低有效（ON=0，软件读取层取反）
- 光 DIP 开关: 2-bit (pin 2/3)，公共端 GND，低有效（ON=0，软件读取层取反）
- UART 排针: UART0 TX (pin 12), RX (pin 11), 3.3V TTL
- 调试接口: SWCLK (pin 18), SWDIO (pin 17)
