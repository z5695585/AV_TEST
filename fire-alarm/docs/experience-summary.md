# Fire Alarm 嵌入式项目经验总结

## 1. 项目概况

| 项 | 值 |
|---|-----|
| MCU | HC32L110C6PA (Cortex-M0+, TSSOP-20, 32KB/4KB) |
| 时钟 | 8MHz 外部晶振 XTH |
| 工具链 | Keil MDK v5 + ARMCC v5.06 + J-Link SWD |
| 外设 | TIM4_CHA (蜂鸣器 PWM), TIM5_CHA (LED PWM), UART0, BT0, DIP×4 |
| 通信 | UART 9600-8-N-1, 文本命令协议 |
| 当前版本 | v0.2 |

---

## 2. 开发时间线与关键节点

### 阶段一：PWM 输出（耗时约 1.5 天）

| # | 现象 | 根因 | 解决 |
|---|------|------|------|
| 1 | PIN1 无波形 | 未调用 `Adt_CHxXPortConfig()` — 定时器在跑但没驱动引脚 | 加 port output config |
| 2 | PIN16 有波形，PIN1 无 | GPIO 时钟未在 `Gpio_SetFunc_XXX` 前使能 | `Clk_SetPeripheralGate(ClkPeripheralGpio, TRUE)` 前置 |
| 3 | 占空比固定在 50%，不可调 | Compare 模式用错 `AdtCHxCompareInv` | 改为 `AdtCHxCompareLow` + `AdtCHxPeriodHigh` |
| 4 | Compare 值修改不生效 | `Adt_EnableValueBuf` 开了缓冲，新值不立即加载 | 移除 Value Buffer |
| 5 | DIP 乱跳 | 浮空引脚 + 无消抖 | 50ms 去抖 + 软件取反 |

**教训**：PWM 不出波形，按顺序排查——先确认定时器在跑（计数寄存器在变），再查端口配置（SEL + DIR + ADS），最后查输出模式（compare/period 极性组合）。

### 阶段二：UART 波特率（耗时约 3 天）⚠️ 本项目最大坑

**现象链**：
1. UART 初始化后 TX 发不出数据
2. 阻塞 `uart_send_str()` 会卡死 → 主循环停止 → PWM 冻结
3. 注释掉 UART TX 则一切正常

**隔离过程**（关键方法）：
```
全关 → OK
uart_init only → OK  
uart_init + blocking send → 死
uart_init + timer_delay(100ms) → OK  ← 证明不是 delay 问题，是 UART TX 本身
```

**根因**：HC32L110 的 UART0 波特率时钟来自 **Base Timer0 (BT0)**。SDK 的 `Uart_SetBaudRate()` 只计算定时器重载值并返回，**不配置定时器本身**。我们的 `uart_init` 从未初始化 BT0，导致 UART 无时钟，TX 的 `while(!TI)` 死等。

更隐蔽的是，BT0 的 **TOG_EN 位必须置 1**——定时器的翻转输出是内部连接到 UART 的时钟线。仅开定时器不够，必须开翻转输出。

**发现过程**：
- 反复对比 SDK example (`uart_master`) 和 SDK debug 代码 (`ddl.c`)
- SDK example 调用 `Bt_Init(TIM1, ...)` 配置定时器，但我们的代码根本没这段
- SDK debug `Debug_UartInit()` 使用 **BT0**（不是 BT1）+ **TOG_EN=1**
- 第一版修复用 BT1 + TOG_EN=0 → 仍不行
- 第二版修复用 BT0 + TOG_EN=1 → TX 终于工作

**教训**：
- SDK 函数名有误导性——`Uart_SetBaudRate` 听起来像完整配置，实则只做一半
- 必须读 SDK 函数源码，不能只看函数名和头文件
- 国产 MCU SDK 文档薄弱，example 代码是最好的参考
- 对比法调试：找到 SDK 里验证可用的代码（`ddl.c` Debug_UartInit），逐行对比差异

### 阶段三：UART 接收丢字节

TX 打通后，RX 又能收到波形但不响应命令。

**根因**：逐字节 echo 回显示波器验证时，每次读一个 byte 后阻塞 1ms 发送 echo，期间新 byte 到来覆盖 SBUF。改为 while 循环一次性读完所有可用字节再处理。

**教训**：9600bps 下每字节 1ms，轮询模式必须快速排空 SBUF，不能在读和读之间做阻塞操作。

### 阶段四：PWM 脉冲首周期异常

改变频率或占空比后，第一个 PWM 周期时长不准（偏宽或偏窄）。

**根因**：`pwm_stop()` → `pwm_set_freq()` → `pwm_set_duty()` → `pwm_start()` 流程中，计数器没清零。停再启后从旧计数值继续数，第一个周期当然不对。

**解决**：封装 `pwm_reconfig(ch, freq, duty)` — **停 → 清零 → 设 period → 设 compare → 启**，原子化执行。

---

## 3. 关键经验原则

### 3.1 不信任 SDK，读源码

国产 MCU SDK 普遍存在：
- 函数名与行为不一致（`Uart_SetBaudRate` 不 set 任何 timer）
- 关键初始化步骤隐藏在 example 里，不在 API 文档里
- 阻塞函数无超时保护（`Uart_SendData` 内部 `while(!TI)` 死等）

**行动准则**：调用任何 SDK 函数前，用 Grep 追踪其实现。对比 SDK example 和我们的调用差异。

### 3.2 原子化寄存器操作

当多个寄存器需要协同修改（如 PWM 的 period + compare），分开写会产生中间态：
```
错误：set period(新) → [中间态：新 period + 旧 compare] → set compare(新)
正确：停 → 清零 → 写 period → 写 compare → 启
```

适用于任何需要多个寄存器保持一致的场景。

### 3.3 隔离法调试

面对 `A+B → 异常` 的情况：
```
A alone → 正常
B alone → 正常
A+B → 异常
```
进一步隔离：
```
A + B_without_X → ?
A + B_without_Y → ?
```
直到找到最小可复现组合。本项目用此法定位到 UART TX 寄存器写入（而非巴率配置或引脚复用）是 PWM 干扰的根因。

### 3.4 Keil 工程文件陷阱

Keil MDK 在打开工程时会覆盖 `.uvprojx` 文件。如果通过外部工具编辑工程文件添加源文件：
- 先关闭 Keil
- 编辑 `.uvprojx`
- 再打开 Keil

或者直接在 Keil UI 里添加文件，避免文件冲突。

---

## 4. Vibe Coding 嵌入式项目复用指南

### 4.1 项目骨架模板

```
project/
├── src/
│   ├── main.c                    # 初始化 + 主循环
│   ├── driver/                   # 驱动头文件（接口层）
│   │   ├── pwm_drv.h
│   │   ├── uart_drv.h
│   │   └── gpio_drv.h
│   ├── hal/<mcu>/                # HAL 实现（硬件层）
│   │   ├── pwm_drv.c
│   │   ├── uart_drv.c
│   │   └── gpio_drv.c
│   ├── app/                      # 应用逻辑
│   │   ├── pattern_engine.c/h    # 状态机引擎
│   │   ├── alarm_sound.c/h       # 声光控制
│   │   ├── alarm_light.c/h
│   │   └── test_protocol.c/h     # 通信协议
│   └── config/
│       └── pattern_table.c/h     # 数据表
├── mdk/                          # Keil 工程
├── CLAUDE.md                     # AI 上下文（重要！）
└── .claude/                      # Claude Code 配置
```

### 4.2 HAL 层设计模式

```c
// 驱动头文件只暴露纯函数接口，不暴露寄存器
void pwm_init(uint8_t channel, uint32_t freq_hz, uint8_t duty_pct);
void pwm_reconfig(uint8_t channel, uint32_t freq_hz, uint8_t duty_pct);
void pwm_start(uint8_t channel);
void pwm_stop(uint8_t channel);
```

好处：
- 应用层不碰寄存器，换 MCU 只改 HAL 层
- AI 容易理解调用意图
- 测试时可以 mock HAL 层

### 4.3 命令协议模式

```c
// 命令表驱动，加命令只需加一行
static const cmd_entry_t default_commands[] = {
    {"PING",   0, h_ping},
    {"FREQ",   2, h_freq},
    ...
};

// 处理流程：收字节 → 行缓冲 → \n 触发 → 查表 → 调 handler
// handler 签名统一：void handler(ctx, argc, argv)
```

适合所有需要串口交互的嵌入式项目。

### 4.4 状态机 + 数据表模式

```c
// 模式不写在代码里，写在数据表里
static const pattern_step_t sound_p0_steps[] = {
    {3000, 25, 500},   // freq, duty%, duration_ms
    {0,    0,  500},   // off step
    ...
};
```

好处：
- 改行为只改表，不改逻辑
- AI 改表比改逻辑更不易出错
- 数据表可视为"嵌入式 DSL"

### 4.5 AI 友好的 CLAUDE.md 写法

```markdown
## Hardware Critical Knowledge
### UART0 Baud Rate
- **Must use BT0 with TOG_EN=1** for baud rate clock
- **DO NOT call SDK Uart_Init/Uart_SendData** — bypassed for a reason

## Architecture
(ASCII 框图展示调用关系)
```

关键原则：
- 把踩过的坑写成 **强制性** 规则（MUST/DO NOT）
- 附上 WHY（不写原因，AI 会自行判断是否可以绕过）
- 用 ASCII 框图而非文字描述模块关系

### 4.6 调试工作流

```
1. 先让 MCU 跑起来 → heartbeat LED 证明主循环存活
2. 一个一个外设点亮 → 先 PWM、后 UART TX、最后 UART RX
3. 每步都做增量测试 → 改最小量，测完再推进
4. 出问题时先隔离 → 二分法排除，不要猜
5. 找到根因后立刻归档 → CLAUDE.md / memory 文件
```

---

## 5. 速查表

### HC32L110 寄存器操作关键点

| 操作 | 正确方式 | 错误方式 |
|------|---------|---------|
| GPIO 功能切换 | 先 `Clk_SetPeripheralGate(Gpio, TRUE)` 再 `Gpio_SetFunc_XXX` | 直接调 `Gpio_SetFunc_XXX` |
| UART 初始化 | 直接写 SCON + BT0 寄存器 | 调 SDK `Uart_Init` / `Uart_SetBaudRate` |
| PWM 参数变更 | `pwm_reconfig(ch, freq, duty)` | 分别调 `pwm_set_freq` + `pwm_set_duty` |
| ADT 端口输出 | `Adt_CHxXPortConfig(enAdt, ch, &cfg)` | 忘记此调用 |
| 读 DIP | 软件取反（common=GND） + 50ms 去抖 | 直接读 |

### UART 通信参数

| 项 | 值 |
|----|-----|
| 波特率 | 9600 |
| 数据位 | 8 |
| 校验 | None |
| 停止位 | 1 |
| 流控 | None |
| 行尾 | `\r\n`（终端须开启 CR+LF） |

---

*文档版本 0.2 — 基于 fire-alarm v0.2 开发过程总结*
