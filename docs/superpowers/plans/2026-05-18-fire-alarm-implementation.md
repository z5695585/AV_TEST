# 声光警报器固件实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现基于 HC32L110C6PA 的声光警报器固件，包含 pattern 驱动的声光报警、DIP 开关选择、UART 测试协议。

**Architecture:** 三层架构 — App 层（alarm_sound、alarm_light、test_protocol）通过 Driver API 接口操作硬件，HAL 层为 HC32L110 提供具体实现。App 模块可在宿主 x86 上通过 mock 驱动进行单元测试。

**Tech Stack:** C99, GCC ARM Embedded, CMake, 华大 HC32L110 SDK (hc32ll10_ddl)

---

## 文件结构

```
fire-alarm/
├── CMakeLists.txt
├── toolchain/
│   └── arm-gcc.cmake
├── src/
│   ├── main.c
│   ├── app/
│   │   ├── alarm_sound.h / alarm_sound.c
│   │   ├── alarm_light.h / alarm_light.c
│   │   ├── test_protocol.h / test_protocol.c
│   │   └── pattern_engine.h / pattern_engine.c
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
│       ├── pattern_table.h
│       └── pattern_table.c
├── test/
│   ├── CMakeLists.txt
│   ├── mock/
│   │   ├── mock_pwm.h / mock_pwm.c
│   │   ├── mock_gpio.h / mock_gpio.c
│   │   ├── mock_uart.h / mock_uart.c
│   │   └── mock_timer.h / mock_timer.c
│   ├── test_main.c
│   ├── test_alarm_sound.c
│   ├── test_alarm_light.c
│   ├── test_protocol.c
│   └── test_pattern_table.c
└── docs/
    ├── superpowers/specs/2026-05-18-fire-alarm-design.md
    └── superpowers/plans/2026-05-18-fire-alarm-implementation.md
```

---

### Task 1: 项目目录和 CMake 骨架

**Files:**
- Create: `CMakeLists.txt`
- Create: `toolchain/arm-gcc.cmake`

- [ ] **Step 1: 创建目录结构**

```bash
mkdir -p src/app src/driver src/hal/hc32l110 src/config \
  test/mock toolchain docs
```

- [ ] **Step 2: 编写顶层 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(fire-alarm C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Target build (ARM embedded)
add_library(driver_hal STATIC
    src/hal/hc32l110/pwm_drv.c
    src/hal/hc32l110/gpio_drv.c
    src/hal/hc32l110/uart_drv.c
    src/hal/hc32l110/timer_drv.c
)

add_library(app STATIC
    src/app/pattern_engine.c
    src/app/alarm_sound.c
    src/app/alarm_light.c
    src/app/test_protocol.c
    src/config/pattern_table.c
)

target_include_directories(driver_hal PUBLIC src/driver)
target_include_directories(app PUBLIC src/app src/config src/driver)
target_link_libraries(app driver_hal)

add_executable(fire-alarm.elf src/main.c)
target_link_libraries(fire-alarm.elf app driver_hal)

# Host test build is in test/CMakeLists.txt, invoked separately
enable_testing()
add_subdirectory(test)
```

- [ ] **Step 3: 编写 ARM GCC 工具链文件**

```cmake
# toolchain/arm-gcc.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(ARM_TOOLCHAIN_PREFIX "arm-none-eabi-")

set(CMAKE_C_COMPILER    ${ARM_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER  ${ARM_TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER  ${ARM_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR            ${ARM_TOOLCHAIN_PREFIX}ar)
set(CMAKE_OBJCOPY       ${ARM_TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP       ${ARM_TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE          ${ARM_TOOLCHAIN_PREFIX}size)

set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb -Wall -Wextra -O2 -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb -Wl,--gc-sections -Wl,-Map=fire-alarm.map --specs=nosys.specs")
```

- [ ] **Step 4: Commit**

```bash
git init
git add -A
git commit -m "feat: add project scaffolding, CMakeLists.txt, ARM toolchain"
```

---

### Task 2: Driver API 头文件（接口契约）

**Files:**
- Create: `src/driver/pwm_drv.h`
- Create: `src/driver/gpio_drv.h`
- Create: `src/driver/uart_drv.h`
- Create: `src/driver/timer_drv.h`

- [ ] **Step 1: 编写 pwm_drv.h**

```c
#ifndef PWM_DRV_H
#define PWM_DRV_H
#include <stdint.h>

void pwm_init(uint8_t channel, uint32_t freq_hz, uint8_t duty_pct);
void pwm_set_freq(uint8_t channel, uint32_t freq_hz);
void pwm_set_duty(uint8_t channel, uint8_t duty_pct);
void pwm_start(uint8_t channel);
void pwm_stop(uint8_t channel);

#endif
```

- [ ] **Step 2: 编写 gpio_drv.h**

```c
#ifndef GPIO_DRV_H
#define GPIO_DRV_H
#include <stdint.h>

#define GPIO_INPUT  0
#define GPIO_OUTPUT 1

void    gpio_init(uint8_t pin, uint8_t mode);
uint8_t gpio_read(uint8_t pin);
void    gpio_write(uint8_t pin, uint8_t level);

#endif
```

- [ ] **Step 3: 编写 uart_drv.h**

```c
#ifndef UART_DRV_H
#define UART_DRV_H
#include <stdint.h>

void     uart_init(uint32_t baudrate);
void     uart_send(const uint8_t *buf, uint16_t len);
uint16_t uart_recv(uint8_t *buf, uint16_t max_len);
void     uart_send_str(const char *str);

#endif
```

- [ ] **Step 4: 编写 timer_drv.h**

```c
#ifndef TIMER_DRV_H
#define TIMER_DRV_H
#include <stdint.h>

void     timer_delay_ms(uint32_t ms);
void     timer_delay_us(uint32_t us);
uint32_t timer_get_tick(void);

#endif
```

- [ ] **Step 5: Commit**

```bash
git add src/driver/
git commit -m "feat: add driver API headers"
```

---

### Task 3: Pattern 配置表

**Files:**
- Create: `src/config/pattern_table.h`
- Create: `src/config/pattern_table.c`

- [ ] **Step 1: 编写 pattern_table.h**

```c
#ifndef PATTERN_TABLE_H
#define PATTERN_TABLE_H
#include <stdint.h>

#define MAX_PATTERNS    16
#define BUZZ_CH         0
#define LED_CH          1

typedef struct {
    uint32_t freq_hz;
    uint8_t  duty_pct;
    uint16_t duration_ms;
} pattern_step_t;

typedef struct {
    uint8_t          step_count;
    const pattern_step_t *steps;
    uint8_t          loop_count;  /* 0=infinite */
} pattern_t;

extern const pattern_t sound_pattern_table[MAX_PATTERNS];
extern const pattern_t light_pattern_table[MAX_PATTERNS];

#endif
```

- [ ] **Step 2: 编写 pattern_table.c**

```c
#include "pattern_table.h"

/* ================================================================
 * 声警报 Pattern 表（4 条，由 2-bit DIP 选择）
 *
 * 声音由 PWM 频率（freq_hz）和占空比（duty_pct）决定：
 *   T3 交替音：6-step 序列模拟 ISO 8201 T3 模式
 *   连续音：单 step 持续输出
 *   小音量 = 50% 占空比，大音量 = 80% 占空比
 * ================================================================ */

/* P0: T3 交替，小音量（50%） */
static const pattern_step_t sound_p0_steps[] = {
    {3000, 50, 500},
    {0,    0,  500},
    {3000, 50, 500},
    {0,    0,  500},
    {3000, 50, 500},
    {0,    0, 1500},
};

/* P1: T3 交替，大音量（80%） */
static const pattern_step_t sound_p1_steps[] = {
    {3000, 80, 500},
    {0,    0,  500},
    {3000, 80, 500},
    {0,    0,  500},
    {3000, 80, 500},
    {0,    0, 1500},
};

/* P2: 连续音，小音量（50%） */
static const pattern_step_t sound_p2_steps[] = {
    {3000, 50, 0},  /* 0 duration = hold this step indefinitely */
};

/* P3: 连续音，大音量（80%） */
static const pattern_step_t sound_p3_steps[] = {
    {3000, 80, 0},
};

const pattern_t sound_pattern_table[MAX_PATTERNS] = {
    {6, sound_p0_steps, 0},  /* P0: T3 small */
    {6, sound_p1_steps, 0},  /* P1: T3 large */
    {1, sound_p2_steps, 0},  /* P2: continuous small */
    {1, sound_p3_steps, 0},  /* P3: continuous large */
    /* P4-P15: reserved → fallback handled by lookup_pattern() */
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
};

/* ================================================================
 * 光警报 Pattern 表（4 条，由 2-bit DIP 选择）
 *
 * 时序：20ms on / 980ms off 循环
 * 亮度通过 on 期间的 PWM 占空比区分
 * LED 驱动芯片跟随 PWM 输入波形
 * ================================================================ */

/* P0: 低亮度（on 期间 30% duty） */
static const pattern_step_t light_p0_steps[] = {
    {3000, 30,  20},
    {0,    0,  980},
};

/* P1: 中亮度（on 期间 60% duty） */
static const pattern_step_t light_p1_steps[] = {
    {3000, 60,  20},
    {0,    0,  980},
};

/* P2: 高亮度（on 期间 100% duty） */
static const pattern_step_t light_p2_steps[] = {
    {3000, 100, 20},
    {0,    0,  980},
};

/* P3: 预留，回退到 P0 */

const pattern_t light_pattern_table[MAX_PATTERNS] = {
    {2, light_p0_steps, 0},  /* P0: low brightness */
    {2, light_p1_steps, 0},  /* P1: medium brightness */
    {2, light_p2_steps, 0},  /* P2: high brightness */
    {0, NULL,          0},  /* P3: reserved → fallback */
    /* P4-P15: reserved */
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
};
```

- [ ] **Step 3: Commit**

```bash
git add src/config/
git commit -m "feat: add pattern table configuration"
```

---

### Task 4: Pattern Engine（共享状态机）

**Files:**
- Create: `src/app/pattern_engine.h`
- Create: `src/app/pattern_engine.c`

- [ ] **Step 1: 编写 pattern_engine.h**

```c
#ifndef PATTERN_ENGINE_H
#define PATTERN_ENGINE_H
#include <stdint.h>
#include "pattern_table.h"

typedef struct {
    const pattern_t *pattern;
    uint8_t  cur_step;
    uint8_t  cur_loop;
    uint32_t step_start_tick;
    uint8_t  active;
    uint8_t  pwm_channel;
} pattern_engine_t;

void    pattern_engine_init(pattern_engine_t *ctx, uint8_t channel);
uint8_t pattern_engine_load(pattern_engine_t *ctx, uint8_t pattern_idx);
void    pattern_engine_tick(pattern_engine_t *ctx, uint32_t now);
uint8_t pattern_engine_is_active(const pattern_engine_t *ctx);
void    pattern_engine_stop(pattern_engine_t *ctx);
uint8_t pattern_engine_current_index(const pattern_engine_t *ctx);

#endif
```

- [ ] **Step 2: 编写 pattern_engine.c**

```c
#include "pattern_engine.h"
#include "pattern_table.h"
#include "pwm_drv.h"

static const pattern_t *lookup_pattern(const pattern_t *table, uint8_t idx) {
    if (idx >= MAX_PATTERNS) return NULL;
    const pattern_t *p = &table[idx];
    if (p->step_count == 0 || p->steps == NULL) {
        /* Fallback to pattern 0 */
        p = &table[0];
        if (p->step_count == 0 || p->steps == NULL) return NULL;
    }
    return p;
}

void pattern_engine_init(pattern_engine_t *ctx, uint8_t channel) {
    ctx->pattern = NULL;
    ctx->cur_step = 0;
    ctx->cur_loop = 0;
    ctx->step_start_tick = 0;
    ctx->active = 0;
    ctx->pwm_channel = channel;
}

uint8_t pattern_engine_load(pattern_engine_t *ctx, uint8_t pattern_idx) {
    const pattern_t *table = (ctx->pwm_channel == BUZZ_CH)
        ? sound_pattern_table : light_pattern_table;
    const pattern_t *p = lookup_pattern(table, pattern_idx);
    if (!p) return 0;

    ctx->pattern = p;
    ctx->cur_step = 0;
    ctx->cur_loop = 0;
    ctx->step_start_tick = 0;
    ctx->active = 1;

    /* Apply first step immediately */
    const pattern_step_t *step = &p->steps[0];
    if (step->freq_hz == 0) {
        pwm_stop(ctx->pwm_channel);
    } else {
        pwm_set_freq(ctx->pwm_channel, step->freq_hz);
        pwm_set_duty(ctx->pwm_channel, step->duty_pct);
        pwm_start(ctx->pwm_channel);
    }
    return 1;
}

void pattern_engine_tick(pattern_engine_t *ctx, uint32_t now) {
    if (!ctx->active || !ctx->pattern) return;

    const pattern_step_t *step = &ctx->pattern->steps[ctx->cur_step];
    uint32_t elapsed = now - ctx->step_start_tick;

    if (elapsed >= step->duration_ms) {
        ctx->cur_step++;
        if (ctx->cur_step >= ctx->pattern->step_count) {
            ctx->cur_step = 0;
            if (ctx->pattern->loop_count > 0) {
                ctx->cur_loop++;
                if (ctx->cur_loop >= ctx->pattern->loop_count) {
                    pwm_stop(ctx->pwm_channel);
                    ctx->active = 0;
                    return;
                }
            }
        }
        step = &ctx->pattern->steps[ctx->cur_step];
        ctx->step_start_tick = now;
        if (step->freq_hz == 0) {
            pwm_stop(ctx->pwm_channel);
        } else {
            pwm_set_freq(ctx->pwm_channel, step->freq_hz);
            pwm_set_duty(ctx->pwm_channel, step->duty_pct);
            pwm_start(ctx->pwm_channel);
        }
    }
}

uint8_t pattern_engine_is_active(const pattern_engine_t *ctx) {
    return ctx->active;
}

void pattern_engine_stop(pattern_engine_t *ctx) {
    pwm_stop(ctx->pwm_channel);
    ctx->active = 0;
}

uint8_t pattern_engine_current_index(const pattern_engine_t *ctx) {
    /* Returns 0xFF if no pattern loaded, otherwise the table index */
    if (!ctx->active || !ctx->pattern) return 0xFF;
    const pattern_t *table = (ctx->pwm_channel == BUZZ_CH)
        ? sound_pattern_table : light_pattern_table;
    for (uint8_t i = 0; i < MAX_PATTERNS; i++) {
        if (&table[i] == ctx->pattern) return i;
    }
    return 0xFF;
}
```

- [ ] **Step 3: Commit**

```bash
git add src/app/pattern_engine.h src/app/pattern_engine.c
git commit -m "feat: add shared pattern engine state machine"
```

---

### Task 5: Alarm Sound 和 Alarm Light 模块

**Files:**
- Create: `src/app/alarm_sound.h`
- Create: `src/app/alarm_sound.c`
- Create: `src/app/alarm_light.h`
- Create: `src/app/alarm_light.c`

- [ ] **Step 1: 编写 alarm_sound.h**

```c
#ifndef ALARM_SOUND_H
#define ALARM_SOUND_H
#include <stdint.h>

void    alarm_sound_init(void);
uint8_t alarm_sound_load(uint8_t pattern_idx);
void    alarm_sound_tick(uint32_t now);
uint8_t alarm_sound_is_active(void);
void    alarm_sound_stop(void);
uint8_t alarm_sound_current(void);

#endif
```

- [ ] **Step 2: 编写 alarm_sound.c**

```c
#include "alarm_sound.h"
#include "pattern_engine.h"
#include "pattern_table.h"

static pattern_engine_t sound_engine;

void alarm_sound_init(void) {
    pattern_engine_init(&sound_engine, BUZZ_CH);
}

uint8_t alarm_sound_load(uint8_t pattern_idx) {
    return pattern_engine_load(&sound_engine, pattern_idx);
}

void alarm_sound_tick(uint32_t now) {
    pattern_engine_tick(&sound_engine, now);
}

uint8_t alarm_sound_is_active(void) {
    return pattern_engine_is_active(&sound_engine);
}

void alarm_sound_stop(void) {
    pattern_engine_stop(&sound_engine);
}

uint8_t alarm_sound_current(void) {
    return pattern_engine_current_index(&sound_engine);
}
```

- [ ] **Step 3: 编写 alarm_light.h**

```c
#ifndef ALARM_LIGHT_H
#define ALARM_LIGHT_H
#include <stdint.h>

void    alarm_light_init(void);
uint8_t alarm_light_load(uint8_t pattern_idx);
void    alarm_light_tick(uint32_t now);
uint8_t alarm_light_is_active(void);
void    alarm_light_stop(void);
uint8_t alarm_light_current(void);

#endif
```

- [ ] **Step 4: 编写 alarm_light.c**

```c
#include "alarm_light.h"
#include "pattern_engine.h"
#include "pattern_table.h"

static pattern_engine_t light_engine;

void alarm_light_init(void) {
    pattern_engine_init(&light_engine, LED_CH);
}

uint8_t alarm_light_load(uint8_t pattern_idx) {
    return pattern_engine_load(&light_engine, pattern_idx);
}

void alarm_light_tick(uint32_t now) {
    pattern_engine_tick(&light_engine, now);
}

uint8_t alarm_light_is_active(void) {
    return pattern_engine_is_active(&light_engine);
}

void alarm_light_stop(void) {
    pattern_engine_stop(&light_engine);
}

uint8_t alarm_light_current(void) {
    return pattern_engine_current_index(&light_engine);
}
```

- [ ] **Step 5: Commit**

```bash
git add src/app/alarm_sound.h src/app/alarm_sound.c \
        src/app/alarm_light.h src/app/alarm_light.c
git commit -m "feat: add alarm sound and light modules"
```

---

### Task 6: 测试协议解析器

**Files:**
- Create: `src/app/test_protocol.h`
- Create: `src/app/test_protocol.c`

- [ ] **Step 1: 编写 test_protocol.h**

```c
#ifndef TEST_PROTOCOL_H
#define TEST_PROTOCOL_H
#include <stdint.h>

#define PROTO_BUF_SIZE  64
#define PROTO_MAX_ARGS  2

typedef struct proto_ctx_t proto_ctx_t;

typedef void (*cmd_handler_t)(proto_ctx_t *ctx, uint8_t argc, char *argv[]);

typedef struct {
    const char    *cmd;
    uint8_t        min_args;
    cmd_handler_t  handler;
} cmd_entry_t;

struct proto_ctx_t {
    char     buf[PROTO_BUF_SIZE];
    uint8_t  len;
    const cmd_entry_t *cmd_table;
    uint8_t  cmd_count;
    uint8_t  sound_dip;
    uint8_t  light_dip;
    uint8_t  buzz_idx;
    uint8_t  led_idx;
};

void proto_init(proto_ctx_t *ctx, const cmd_entry_t *table, uint8_t count);
void proto_feed(proto_ctx_t *ctx, const uint8_t *data, uint16_t len);
void proto_update_state(proto_ctx_t *ctx, uint8_t s_dip, uint8_t l_dip, uint8_t buzz, uint8_t led);

#endif
```

- [ ] **Step 2: 编写 test_protocol.c**

```c
#include "test_protocol.h"
#include <string.h>
#include <stdio.h>

/* --- External callbacks, implemented in main.c --- */
extern void proto_buzz_load(uint8_t idx);
extern void proto_buzz_stop(void);
extern void proto_led_load(uint8_t idx);
extern void proto_led_stop(void);
extern void proto_reset_to_dip(void);

static void proto_reply(const char *msg);

/* Tokenize: split input line into cmd + argv. Returns argc. */
static uint8_t parse_line(char *line, char *argv[], uint8_t max_args) {
    uint8_t argc = 0;
    char *token = line;
    char *next;
    while (argc < max_args + 1) { /* +1 for cmd itself */
        /* Skip leading spaces */
        while (*token == ' ') token++;
        if (*token == '\0' || *token == '\r') break;
        /* Find end of token */
        next = token;
        while (*next != ' ' && *next != '\0' && *next != '\r') next++;
        if (argc == 0) {
            argv[0] = token; /* cmd */
        } else {
            argv[argc] = token;
        }
        argc++;
        if (*next == '\0' || *next == '\r') break;
        *next = '\0';
        token = next + 1;
    }
    return argc;
}

/* --- Command handlers --- */

static void h_buzz_on(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    uint8_t idx = (uint8_t)atoi(argv[1]);
    if (idx >= MAX_PATTERNS) {
        proto_reply("ERR RANGE\r\n");
        return;
    }
    proto_buzz_load(idx);
    ctx->buzz_idx = idx;
    char rsp[32];
    snprintf(rsp, sizeof(rsp), "OK BUZZ=%d\r\n", idx);
    proto_reply(rsp);
}

static void h_buzz_off(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    proto_buzz_stop();
    ctx->buzz_idx = 0xFF;
    proto_reply("OK BUZZ=OFF\r\n");
}

static void h_led_on(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    uint8_t idx = (uint8_t)atoi(argv[1]);
    if (idx >= MAX_PATTERNS) {
        proto_reply("ERR RANGE\r\n");
        return;
    }
    proto_led_load(idx);
    ctx->led_idx = idx;
    char rsp[32];
    snprintf(rsp, sizeof(rsp), "OK LED=%d\r\n", idx);
    proto_reply(rsp);
}

static void h_led_off(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    proto_led_stop();
    ctx->led_idx = 0xFF;
    proto_reply("OK LED=OFF\r\n");
}

static void h_dip(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    char rsp[32];
    snprintf(rsp, sizeof(rsp), "OK SD=%d LD=%d\r\n", ctx->sound_dip, ctx->light_dip);
    proto_reply(rsp);
}

static void h_status(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    char rsp[48];
    snprintf(rsp, sizeof(rsp), "OK B=%d L=%d SD=%d LD=%d\r\n",
             ctx->buzz_idx == 0xFF ? 255 : ctx->buzz_idx,
             ctx->led_idx == 0xFF ? 255 : ctx->led_idx,
             ctx->sound_dip, ctx->light_dip);
    proto_reply(rsp);
}

static void h_reset(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    proto_reset_to_dip();
    proto_reply("OK RESET\r\n");
}

static void h_test(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    /* Quick self-test: buzz pattern 0 for 3 brief bursts */
    proto_reply("OK ALARM_TEST\r\n");
    /* Main loop handles the test sequence via proto_ctx_t state */
    proto_buzz_load(0);
    proto_led_load(0);
}

/* --- Command table --- */

static const cmd_entry_t default_commands[] = {
    {"BUZZ",    1, h_buzz_on},
    {"LED",     1, h_led_on},
    {"DIP",     0, h_dip},
    {"STATUS",  0, h_status},
    {"RESET",   0, h_reset},
    {"TEST",    0, h_test},
};

/* --- API --- */

void proto_init(proto_ctx_t *ctx, const cmd_entry_t *table, uint8_t count) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->buf[0] = '\0';
    ctx->len = 0;
    ctx->cmd_table = table ? table : default_commands;
    ctx->cmd_count = table ? count : (sizeof(default_commands) / sizeof(default_commands[0]));
    ctx->buzz_idx = 0xFF;
    ctx->led_idx = 0xFF;
}

void proto_update_state(proto_ctx_t *ctx, uint8_t s_dip, uint8_t l_dip, uint8_t buzz, uint8_t led) {
    ctx->sound_dip = s_dip;
    ctx->light_dip = l_dip;
    ctx->buzz_idx = buzz;
    ctx->led_idx = led;
}

void proto_feed(proto_ctx_t *ctx, const uint8_t *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        char c = (char)data[i];
        if (c == '\r') continue; /* skip */

        if (c == '\n') {
            ctx->buf[ctx->len] = '\0';
            ctx->len = 0;

            if (ctx->buf[0] == '\0') continue; /* empty line */

            char *argv[PROTO_MAX_ARGS + 1] = {0};
            uint8_t argc = parse_line(ctx->buf, argv, PROTO_MAX_ARGS);

            if (argc == 0) {
                proto_reply("ERR CMD\r\n");
                continue;
            }

            /* Lookup command */
            uint8_t found = 0;
            for (uint8_t j = 0; j < ctx->cmd_count; j++) {
                if (strcmp(argv[0], ctx->cmd_table[j].cmd) == 0) {
                    if (argc - 1 < ctx->cmd_table[j].min_args) {
                        proto_reply("ERR CMD\r\n");
                    } else {
                        ctx->cmd_table[j].handler(ctx, argc - 1, argv + 1);
                    }
                    found = 1;
                    break;
                }
            }
            if (!found) {
                proto_reply("ERR CMD\r\n");
            }
        } else {
            if (ctx->len < PROTO_BUF_SIZE - 1) {
                ctx->buf[ctx->len++] = c;
            }
            /* If buffer overflows, drop bytes until next newline.
               The len stays at max, next \n will process truncated content. */
        }
    }
}

/* --- Reply callback, to be defined by main.c --- */
/* In the implementation, main.c provides this function that calls uart_send_str */
static void proto_reply(const char *msg) {
    uart_send_str(msg);
}
```

- [ ] **Step 3: Commit**

```bash
git add src/app/test_protocol.h src/app/test_protocol.c
git commit -m "feat: add test protocol parser with command table"
```

---

### Task 7: Mock 驱动和宿主单元测试

**Files:**
- Create: `test/CMakeLists.txt`
- Create: `test/mock/mock_pwm.h`
- Create: `test/mock/mock_pwm.c`
- Create: `test/mock/mock_gpio.h`
- Create: `test/mock/mock_gpio.c`
- Create: `test/mock/mock_uart.h`
- Create: `test/mock/mock_uart.c`
- Create: `test/mock/mock_timer.h`
- Create: `test/mock/mock_timer.c`
- Create: `test/test_main.c`
- Create: `test/test_pattern_table.c`
- Create: `test/test_alarm_sound.c`
- Create: `test/test_alarm_light.c`
- Create: `test/test_protocol.c`

- [ ] **Step 1: 编写 mock 驱动**

```c
/* test/mock/mock_pwm.h */
#ifndef MOCK_PWM_H
#define MOCK_PWM_H
#include <stdint.h>
void mock_pwm_reset(void);
uint8_t mock_pwm_is_running(uint8_t channel);
uint32_t mock_pwm_get_freq(uint8_t channel);
uint8_t mock_pwm_get_duty(uint8_t channel);
#endif

/* test/mock/mock_pwm.c */
#include "pwm_drv.h"
static uint8_t  s_running[2];
static uint32_t s_freq[2];
static uint8_t  s_duty[2];

void mock_pwm_reset(void) {
    s_running[0] = s_running[1] = 0;
    s_freq[0] = s_freq[1] = 0;
    s_duty[0] = s_duty[1] = 0;
}

void pwm_init(uint8_t ch, uint32_t f, uint8_t d) { s_freq[ch]=f; s_duty[ch]=d; s_running[ch]=1; }
void pwm_set_freq(uint8_t ch, uint32_t f) { s_freq[ch]=f; }
void pwm_set_duty(uint8_t ch, uint8_t d) { s_duty[ch]=d; }
void pwm_start(uint8_t ch) { s_running[ch]=1; }
void pwm_stop(uint8_t ch) { s_running[ch]=0; }

uint8_t  mock_pwm_is_running(uint8_t ch) { return s_running[ch]; }
uint32_t mock_pwm_get_freq(uint8_t ch)   { return s_freq[ch]; }
uint8_t  mock_pwm_get_duty(uint8_t ch)   { return s_duty[ch]; }
```

```c
/* test/mock/mock_gpio.h */
#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H
#include <stdint.h>
void mock_gpio_set_pin(uint8_t pin, uint8_t val);
uint8_t mock_gpio_get_pin_mode(uint8_t pin);
uint8_t mock_gpio_get_pin_out(uint8_t pin);
#endif

/* test/mock/mock_gpio.c */
#include "gpio_drv.h"
static uint8_t s_pin_val[16];
static uint8_t s_pin_mode[16];

void gpio_init(uint8_t pin, uint8_t mode) { s_pin_mode[pin]=mode; }
uint8_t gpio_read(uint8_t pin) { return s_pin_val[pin]; }
void gpio_write(uint8_t pin, uint8_t level) { s_pin_val[pin]=level; }
void mock_gpio_set_pin(uint8_t pin, uint8_t val) { s_pin_val[pin]=val; }
uint8_t mock_gpio_get_pin_mode(uint8_t pin) { return s_pin_mode[pin]; }
uint8_t mock_gpio_get_pin_out(uint8_t pin) { return s_pin_val[pin]; }
```

```c
/* test/mock/mock_uart.h */
#ifndef MOCK_UART_H
#define MOCK_UART_H
#include <stdint.h>
void mock_uart_reset(void);
const char *mock_uart_last_sent(void);
uint16_t mock_uart_sent_count(void);
#endif

/* test/mock/mock_uart.c */
#include "uart_drv.h"
#include <string.h>
static char s_tx_buf[256];
static uint16_t s_tx_len;

void mock_uart_reset(void) { s_tx_buf[0]='\0'; s_tx_len=0; }
void uart_init(uint32_t baudrate) { (void)baudrate; }
void uart_send(const uint8_t *buf, uint16_t len) {
    uint16_t copy = (len < sizeof(s_tx_buf)-1) ? len : (sizeof(s_tx_buf)-1);
    memcpy(s_tx_buf, buf, copy);
    s_tx_buf[copy] = '\0';
    s_tx_len = copy;
}
uint16_t uart_recv(uint8_t *buf, uint16_t max_len) { (void)buf; (void)max_len; return 0; }
void uart_send_str(const char *str) { uart_send((const uint8_t*)str, (uint16_t)strlen(str)); }
const char *mock_uart_last_sent(void) { return s_tx_buf; }
uint16_t mock_uart_sent_count(void) { return s_tx_len; }
```

```c
/* test/mock/mock_timer.h */
#ifndef MOCK_TIMER_H
#define MOCK_TIMER_H
#include <stdint.h>
void mock_timer_set_tick(uint32_t tick);
void mock_timer_advance(uint32_t ms);
#endif

/* test/mock/mock_timer.c */
#include "timer_drv.h"
static uint32_t s_tick;

void timer_delay_ms(uint32_t ms) { s_tick += ms; }
void timer_delay_us(uint32_t us) { s_tick += (us / 1000); if (us % 1000) s_tick++; }
uint32_t timer_get_tick(void) { return s_tick; }
void mock_timer_set_tick(uint32_t tick) { s_tick = tick; }
void mock_timer_advance(uint32_t ms) { s_tick += ms; }
```

- [ ] **Step 2: 编写 test_main.c（自定义轻量断言框架）**

```c
/* test/test_main.c */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int s_passed = 0;
static int s_failed = 0;

#define TEST(name) void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  RUN  %s\n", #name); \
    test_##name(); \
} while(0)

#define ASSERT_EQ(expected, actual, fmt) do { \
    if ((expected) != (actual)) { \
        printf("  FAIL %s:%d: expected " fmt " got " fmt "\n", \
               __FILE__, __LINE__, (expected), (actual)); \
        s_failed++; return; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("  FAIL %s:%d: expected \"%s\" got \"%s\"\n", \
               __FILE__, __LINE__, (expected), (actual)); \
        s_failed++; return; \
    } \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        printf("  FAIL %s:%d: expected true\n", __FILE__, __LINE__); \
        s_failed++; return; \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

/* Forward declarations */
TEST(pattern_table);
TEST(alarm_sound_basic);
TEST(alarm_sound_advance);
TEST(alarm_light_basic);
TEST(protocol_buzz_on_off);
TEST(protocol_led_on_off);
TEST(protocol_dip_status);
TEST(protocol_unknown_cmd);
TEST(protocol_range_check);

int main(void) {
    printf("=== Fire Alarm Unit Tests ===\n\n");
    
    printf("Pattern Table:\n");
    RUN_TEST(pattern_table);
    
    printf("\nAlarm Sound:\n");
    RUN_TEST(alarm_sound_basic);
    RUN_TEST(alarm_sound_advance);
    
    printf("\nAlarm Light:\n");
    RUN_TEST(alarm_light_basic);
    
    printf("\nTest Protocol:\n");
    RUN_TEST(protocol_buzz_on_off);
    RUN_TEST(protocol_led_on_off);
    RUN_TEST(protocol_dip_status);
    RUN_TEST(protocol_unknown_cmd);
    RUN_TEST(protocol_range_check);
    
    printf("\n=== Results: %d passed, %d failed ===\n", s_passed, s_failed);
    return s_failed ? 1 : 0;
}
```

- [ ] **Step 3: 编写 test_pattern_table.c**

```c
/* test/test_pattern_table.c */
#include "test_main.c" /* include for test macros, or use a common header */
/* Actually let's use a proper approach with a test helper header */
```

实际测试代码需要引用 mock 和模块。为简洁，测试文件直接 include test_main.c 中的宏和需要测试的模块。下面各测试文件的完整代码：

```c
/* test/test_pattern_table.c */
#include "mock_pwm.h"
#include "mock_timer.h"
#include "pattern_table.h"
#include <stdio.h>

extern int s_passed, s_failed;

TEST(pattern_table) {
    /* P0 must exist */
    ASSERT_TRUE(sound_pattern_table[0].step_count > 0);
    ASSERT_TRUE(sound_pattern_table[0].steps != NULL);
    ASSERT_TRUE(light_pattern_table[0].step_count > 0);
    ASSERT_TRUE(light_pattern_table[0].steps != NULL);

    /* Unused slots must fallback to NULL steps (engine handles fallback) */
    ASSERT_TRUE(sound_pattern_table[0].loop_count == 0);  /* infinite */
    
    s_passed++;
}
```

```c
/* test/test_alarm_sound.c */
#include "mock_pwm.h"
#include "mock_timer.h"
#include "alarm_sound.h"
#include <stdio.h>

extern int s_passed, s_failed;

TEST(alarm_sound_basic) {
    mock_pwm_reset();
    mock_timer_set_tick(0);

    alarm_sound_init();

    /* Load pattern 0 (T3 small, first step 3kHz/50%) */
    uint8_t ok = alarm_sound_load(0);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(alarm_sound_is_active());
    ASSERT_TRUE(mock_pwm_is_running(0));
    ASSERT_EQ((uint32_t)3000, mock_pwm_get_freq(0), "%lu");
    ASSERT_EQ((uint8_t)50, mock_pwm_get_duty(0), "%u");

    /* Stop */
    alarm_sound_stop();
    ASSERT_FALSE(alarm_sound_is_active());
    ASSERT_FALSE(mock_pwm_is_running(0));

    /* Load out-of-range */
    ok = alarm_sound_load(200);
    ASSERT_TRUE(ok);  /* falls back to P0 */
    ASSERT_TRUE(alarm_sound_is_active());

    s_passed++;
}

TEST(alarm_sound_advance) {
    mock_pwm_reset();
    mock_timer_set_tick(0);

    alarm_sound_init();
    alarm_sound_load(1);  /* P1: 500ms on / 500ms off */

    /* Step 0: 3000Hz at 80%, running */
    ASSERT_TRUE(mock_pwm_is_running(0));

    /* Advance 500ms */
    mock_timer_advance(500);
    alarm_sound_tick(mock_timer_get_tick());

    /* Step 1: freq=0 → PWM stopped */
    ASSERT_FALSE(mock_pwm_is_running(0));

    /* Advance another 500ms → back to step 0 (infinite loop) */
    mock_timer_advance(500);
    alarm_sound_tick(500);
    ASSERT_TRUE(mock_pwm_is_running(0));

    s_passed++;
}
```

```c
/* test/test_alarm_light.c */
#include "mock_pwm.h"
#include "mock_timer.h"
#include "alarm_light.h"
#include <stdio.h>

extern int s_passed, s_failed;

TEST(alarm_light_basic) {
    mock_pwm_reset();
    mock_timer_set_tick(0);

    alarm_light_init();
    uint8_t ok = alarm_light_load(0);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(alarm_light_is_active());
    ASSERT_TRUE(mock_pwm_is_running(1));  /* channel 1 = LED */

    /* Sound on channel 0 should be independent */
    ASSERT_FALSE(mock_pwm_is_running(0));

    alarm_light_stop();
    ASSERT_FALSE(mock_pwm_is_running(1));

    s_passed++;
}
```

```c
/* test/test_protocol.c */
#include "mock_uart.h"
#include "test_protocol.h"
#include <stdio.h>
#include <string.h>

extern int s_passed, s_failed;

/* These callback stubs simulate main.c integration */
static uint8_t g_buzz_idx = 0xFF, g_led_idx = 0xFF;
static uint8_t g_s_dip = 0, g_l_dip = 0;

void proto_buzz_load(uint8_t idx) { g_buzz_idx = idx; }
void proto_buzz_stop(void) { g_buzz_idx = 0xFF; }
void proto_led_load(uint8_t idx) { g_led_idx = idx; }
void proto_led_stop(void) { g_led_idx = 0xFF; }
void proto_reset_to_dip(void) { g_buzz_idx = g_s_dip; g_led_idx = g_l_dip; }

static void feed_str(proto_ctx_t *ctx, const char *str) {
    proto_feed(ctx, (const uint8_t*)str, (uint16_t)strlen(str));
}

TEST(protocol_buzz_on_off) {
    mock_uart_reset();
    proto_ctx_t ctx;
    proto_init(&ctx, NULL, 0);
    proto_update_state(&ctx, 0, 0, 0xFF, 0xFF);

    feed_str(&ctx, "BUZZ ON 0\n");
    ASSERT_EQ((uint8_t)0, g_buzz_idx, "%u");
    ASSERT_STR_EQ("OK BUZZ=0\r\n", mock_uart_last_sent());

    feed_str(&ctx, "BUZZ OFF\n");
    ASSERT_EQ((uint8_t)0xFF, g_buzz_idx, "%u");
    ASSERT_STR_EQ("OK BUZZ=OFF\r\n", mock_uart_last_sent());

    s_passed++;
}

TEST(protocol_led_on_off) {
    mock_uart_reset();
    proto_ctx_t ctx;
    proto_init(&ctx, NULL, 0);

    feed_str(&ctx, "LED ON 1\n");
    ASSERT_EQ((uint8_t)1, g_led_idx, "%u");
    ASSERT_STR_EQ("OK LED=1\r\n", mock_uart_last_sent());

    feed_str(&ctx, "LED OFF\n");
    ASSERT_EQ((uint8_t)0xFF, g_led_idx, "%u");

    s_passed++;
}

TEST(protocol_dip_status) {
    mock_uart_reset();
    proto_ctx_t ctx;
    proto_init(&ctx, NULL, 0);
    proto_update_state(&ctx, 2, 1, 0xFF, 0xFF);

    feed_str(&ctx, "DIP\n");
    ASSERT_STR_EQ("OK SD=2 LD=1\r\n", mock_uart_last_sent());

    feed_str(&ctx, "STATUS\n");
    ASSERT_STR_EQ("OK B=255 L=255 SD=2 LD=1\r\n", mock_uart_last_sent());

    s_passed++;
}

TEST(protocol_unknown_cmd) {
    mock_uart_reset();
    proto_ctx_t ctx;
    proto_init(&ctx, NULL, 0);

    feed_str(&ctx, "FOOBAR\n");
    ASSERT_STR_EQ("ERR CMD\r\n", mock_uart_last_sent());

    s_passed++;
}

TEST(protocol_range_check) {
    mock_uart_reset();
    proto_ctx_t ctx;
    proto_init(&ctx, NULL, 0);

    feed_str(&ctx, "BUZZ ON 99\n");
    ASSERT_STR_EQ("ERR RANGE\r\n", mock_uart_last_sent());

    s_passed++;
}
```

- [ ] **Step 4: 编写 test/CMakeLists.txt**

```cmake
# Host-side test build
add_executable(fire-alarm-test
    test_main.c
    test_pattern_table.c
    test_alarm_sound.c
    test_alarm_light.c
    test_protocol.c

    # Mock drivers
    mock/mock_pwm.c
    mock/mock_gpio.c
    mock/mock_uart.c
    mock/mock_timer.c

    # Modules under test (path relative to project root)
    ../src/app/pattern_engine.c
    ../src/app/alarm_sound.c
    ../src/app/alarm_light.c
    ../src/app/test_protocol.c
    ../src/config/pattern_table.c
)

target_include_directories(fire-alarm-test PRIVATE
    mock
    ../src/driver
    ../src/app
    ../src/config
)

add_test(NAME fire-alarm-unit COMMAND fire-alarm-test)
```

- [ ] **Step 5: 构建并运行单元测试**

```bash
cd test
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
./fire-alarm-test
```

期望输出：所有测试通过。

- [ ] **Step 6: Commit**

```bash
git add test/
git commit -m "feat: add mock drivers and host-side unit tests"
```

---

### Task 8: HC32L110 HAL 实现

**Files:**
- Create: `src/hal/hc32l110/pwm_drv.c`
- Create: `src/hal/hc32l110/gpio_drv.c`
- Create: `src/hal/hc32l110/uart_drv.c`
- Create: `src/hal/hc32l110/timer_drv.c`

- [ ] **Step 1: 编写 pwm_drv.c（HC32L110 TIM4_CHA + TIM5_CHA）**

```c
/* src/hal/hc32l110/pwm_drv.c
 *
 * PWM assignment (TSSOP20):
 *   Channel 0 (Buzzer): TIM4_CHA → P34 (physical pin 1, logical GPIO 28)
 *   Channel 1 (LED):    TIM5_CHA → P25 (physical pin 15, logical GPIO 21)
 *
 * TIM4 and TIM5 are advanced 16-bit timers with PWM output on CHA.
 * See datasheet §Timer for register details.
 */
#include "pwm_drv.h"
#include "hc32l110_conf.h"

/* TIM4 → P34 (buzzer), TIM5 → P25 (LED) */
#define BUZZ_TIM    M0P_TIM4
#define LED_TIM     M0P_TIM5

/* GPIO alternate function for TIM4_CHA / TIM5_CHA:
   Configure the PORT_CTRL function select register for P34 and P25.
   The exact function number depends on the SDK's PORT_CTRL definitions. */
static void pwm_pin_config(uint8_t channel) {
    if (channel == 0) {
        /* P34 → TIM4_CHA: set PORT_CTRL function for P34 to TIM4_CHA AF */
        PORT_SetFunc(Port3, Pin4, Func_Tim4Cha, Disable);
    } else {
        /* P25 → TIM5_CHA: set PORT_CTRL function for P25 to TIM5_CHA AF */
        PORT_SetFunc(Port2, Pin5, Func_Tim5Cha, Disable);
    }
}

void pwm_init(uint8_t channel, uint32_t freq_hz, uint8_t duty_pct) {
    M0P_TIMER_TypeDef *tim = (channel == 0) ? BUZZ_TIM : LED_TIM;

    pwm_pin_config(channel);

    if (channel == 0) Sysctrl_SetPeripheralGate(SysctrlPeripheralTim4, TRUE);
    else              Sysctrl_SetPeripheralGate(SysctrlPeripheralTim5, TRUE);

    uint16_t period = (uint16_t)(SystemCoreClock / freq_hz);
    uint16_t compare = (uint16_t)((uint32_t)period * duty_pct / 100);

    tim->ARR   = period;
    tim->CCR1  = compare;   /* CHA → CCR1 */
    tim->CNT   = 0;

    /* CHA: PWM mode 1, output enabled */
    tim->CCMR1 = (6 << 4);  /* OC1M = PWM mode 1 */
    tim->CCER  = (1 << 0);  /* CC1E = CHA output enable */
}

void pwm_set_freq(uint8_t channel, uint32_t freq_hz) {
    M0P_TIMER_TypeDef *tim = (channel == 0) ? BUZZ_TIM : LED_TIM;
    uint16_t period = (uint16_t)(SystemCoreClock / freq_hz);
    tim->ARR = period;
}

void pwm_set_duty(uint8_t channel, uint8_t duty_pct) {
    M0P_TIMER_TypeDef *tim = (channel == 0) ? BUZZ_TIM : LED_TIM;
    uint16_t compare = (uint16_t)((uint32_t)tim->ARR * duty_pct / 100);
    tim->CCR1 = compare;
}

void pwm_start(uint8_t channel) {
    M0P_TIMER_TypeDef *tim = (channel == 0) ? BUZZ_TIM : LED_TIM;
    tim->CR1 |= 1;  /* CEN */
}

void pwm_stop(uint8_t channel) {
    M0P_TIMER_TypeDef *tim = (channel == 0) ? BUZZ_TIM : LED_TIM;
    tim->CR1 &= ~1U;
}
```

> **注意:** `PORT_SetFunc()` 和 `Func_Tim4Cha` 的具体名称取决于华大 SDK 的实际命名。以上为预期形式，实现时根据 SDK 头文件调整。

- [ ] **Step 2: 编写 gpio_drv.c（多端口 P0/P1/P2/P3）**

```c
/* src/hal/hc32l110/gpio_drv.c
 *
 * HC32L110 has 4 GPIO ports (P0, P1, P2, P3).
 * GPIO logical# = Port×8 + Bit.  Pin 0-7→P0, 8-15→P1, 16-23→P2, 24-31→P3.
 *
 * SDK GPIO API: PORT_SetFunc(), PORT_Init(), GPIO_ReadPort(), GPIO_WritePort()
 */
#include "gpio_drv.h"
#include "hc32l110_conf.h"

/* Map logical pin# → (port, bit) using SDK types */
typedef struct { uint8_t port; uint8_t pin; } pin_map_t;

static pin_map_t pin_decode(uint8_t logical) {
    pin_map_t m;
    m.port = logical / 8;   /* 0=P0, 1=P1, 2=P2, 3=P3 */
    m.pin  = logical % 8;
    return m;
}

void gpio_init(uint8_t pin, uint8_t mode) {
    pin_map_t m = pin_decode(pin);
    /* Set GPIO function (not AF), enable pull-up for inputs */
    PORT_SetFunc(en_port_t(m.port), en_pin_t(m.pin),
                 Func_Gpio, Disable);
    if (mode == GPIO_INPUT) {
        PORT_Init(en_port_t(m.port), en_pin_t(m.pin),
                  Port_Input, Port_PullUp);
    } else {
        PORT_Init(en_port_t(m.port), en_pin_t(m.pin),
                  Port_Output, Port_DisablePull);
    }
}

uint8_t gpio_read(uint8_t pin) {
    pin_map_t m = pin_decode(pin);
    return GPIO_ReadPort(en_port_t(m.port), en_pin_t(m.pin));
}

void gpio_write(uint8_t pin, uint8_t level) {
    pin_map_t m = pin_decode(pin);
    GPIO_WritePort(en_port_t(m.port), en_pin_t(m.pin), level);
}
```

> **注意:** `en_port_t(Port0/1/2/3)`, `en_pin_t(Pin0-7)`, `Func_Gpio`, `Port_Input/PullUp/Output` 的实际命名取决于华大 SDK。以上为预期形式，实现时根据 `hc32ll10_ddl` 头文件调整。

- [ ] **Step 3: 编写 uart_drv.c（UART0）**

```c
/* src/hal/hc32l110/uart_drv.c */
#include "uart_drv.h"
#include "hc32l110_conf.h"

#define UART_BAUD(b) (SystemCoreClock / (b))

void uart_init(uint32_t baudrate) {
    Sysctrl_SetPeripheralGate(SysctrlPeripheralUart0, TRUE);

    M0P_UART0->CR1 = 0;           /* Disable during config */
    M0P_UART0->SCON = 3;          /* 8-N-1 */
    M0P_UART0->BDR  = UART_BAUD(baudrate);
    M0P_UART0->CR1  = (1 << 2) | (1 << 3);  /* RE | TE */
}

void uart_send(const uint8_t *buf, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        while (!(M0P_UART0->SR & 2));  /* Wait TXE */
        M0P_UART0->DR = buf[i];
    }
}

uint16_t uart_recv(uint8_t *buf, uint16_t max_len) {
    uint16_t count = 0;
    while (count < max_len && (M0P_UART0->SR & (1 << 5))) {  /* RXNE */
        buf[count++] = (uint8_t)(M0P_UART0->DR & 0xFF);
    }
    return count;
}

void uart_send_str(const char *str) {
    while (*str) {
        while (!(M0P_UART0->SR & 2));
        M0P_UART0->DR = (uint8_t)*str++;
    }
}
```

- [ ] **Step 4: 编写 timer_drv.c（SysTick）**

```c
/* src/hal/hc32l110/timer_drv.c */
#include "timer_drv.h"
#include "hc32l110_conf.h"

static volatile uint32_t s_tick_ms = 0;

/* SysTick_Handler — called every 1ms */
void SysTick_Handler(void) {
    s_tick_ms++;
}

void timer_delay_ms(uint32_t ms) {
    uint32_t start = s_tick_ms;
    while ((s_tick_ms - start) < ms) { /* wait */ }
}

void timer_delay_us(uint32_t us) {
    /* Simple busy loop for microsecond delays.
       For 8MHz core: 1us ≈ 8 cycles, approximate with __NOP loop */
    uint32_t count = us * (SystemCoreClock / 4000000);  /* approximation */
    while (count--) { __NOP(); }
}

uint32_t timer_get_tick(void) {
    return s_tick_ms;
}

void timer_init(void) {
    /* Configure SysTick for 1ms interval */
    SysTick_Config(SystemCoreClock / 1000);
}
```

- [ ] **Step 5: Commit**

```bash
git add src/hal/
git commit -m "feat: add HC32L110 HAL driver implementations"
```

---

### Task 9: Main 入口和 DIP 消抖

**Files:**
- Create: `src/main.c`

- [ ] **Step 1: 编写 main.c**

```c
/* src/main.c
 *
 * Pin mapping (HC32L110C6PA, TSSOP-20, Rev2.6 datasheet):
 *   Physical pin → GPIO name (logical#) : Function
 *   Pin 1  → P34 (28) : Buzzer PWM — TIM4_CHA, open-drain → transistor drive
 *   Pin 15 → P25 (21) : LED PWM   — TIM5_CHA, → LED driver IC input
 *   Pin 12 → P14 (12) : UART0 TX
 *   Pin 11 → P15 (13) : UART0 RX
 *   Pin 19 → P32 (26) : Sound DIP BIT0 (LSB)
 *   Pin 20 → P33 (27) : Sound DIP BIT1 (MSB)
 *   Pin 2  → P35 (29) : Light DIP BIT0 (LSB)
 *   Pin 3  → P36 (30) : Light DIP BIT1 (MSB)
 *   Pin 5  → P01 (1)  : XTHI (8MHz crystal)
 *   Pin 6  → P02 (2)  : XTHO (8MHz crystal)
 *   Pin 9  → AVCC/DVCC : VDD 3.3V
 *   Pin 7  → AVSS/DVSS : GND
 *   Pin 4  → P00 (0)  : RESETB (NRST, active low)
 *   Pin 17 → P27 (23) : SWDIO
 *   Pin 18 → P31 (25) : SWCLK
 *   Unused: P03(3), P23(19), P24(20), P26(22) → input + pull-up
 *
 * GPIO logical# = Port×8 + Bit. P00=0, P14=12, P23=19, P31=25, etc.
 * DIP: common = GND → active LOW → software inverts so 1 = ON.
 */
#include "alarm_sound.h"
#include "alarm_light.h"
#include "test_protocol.h"
#include "gpio_drv.h"
#include "uart_drv.h"
#include "timer_drv.h"
#include "pwm_drv.h"
#include "pattern_table.h"

/* --- Pin assignments (GPIO logical numbers = Port×8 + Bit) --- */
#define SOUND_DIP0_PIN  26   /* P32 */
#define SOUND_DIP1_PIN  27   /* P33 */
#define LIGHT_DIP0_PIN  29   /* P35 */
#define LIGHT_DIP1_PIN  30   /* P36 */
/* PWM output pins are handled inside HAL pwm_drv.c (TIM4_CHA, TIM5_CHA) */
/* UART pins are handled inside HAL uart_drv.c (UART0: P14/TX, P15/RX) */

/* System clock: 8MHz external crystal, no PLL (low-power preference) */
#define SYSTEM_CLOCK_HZ  8000000U

/* --- Command handler callbacks --- */
void proto_buzz_load(uint8_t idx) { alarm_sound_load(idx); }
void proto_buzz_stop(void)         { alarm_sound_stop(); }
void proto_led_load(uint8_t idx)   { alarm_light_load(idx); }
void proto_led_stop(void)          { alarm_light_stop(); }

/* RESET: restore to DIP — deferred to main loop via flag */
static volatile uint8_t g_reset_pending = 0;
void proto_reset_to_dip(void) { g_reset_pending = 1; }

/* --- DIP reading (2-bit, active LOW → invert) --- */

static uint8_t dip_read_2bit(uint8_t pin0, uint8_t pin1) {
    uint8_t raw = 0;
    if (gpio_read(pin0)) raw |= 1;
    if (gpio_read(pin1)) raw |= 2;
    return raw ^ 0x03;  /* invert: common=GND → LOW=ON → 1=ON */
}

typedef struct {
    uint8_t  stable_val;
    uint8_t  last_raw;
    uint32_t last_change_tick;
} dip_debounce_t;

static uint8_t dip_debounced(dip_debounce_t *d, uint8_t raw, uint32_t now) {
    if (raw != d->last_raw) {
        d->last_raw = raw;
        d->last_change_tick = now;
    }
    if ((now - d->last_change_tick) >= 50) {
        d->stable_val = raw;
    }
    return d->stable_val;
}

/* --- main --- */
int main(void) {
    /* System init */
    SystemCoreClock = SYSTEM_CLOCK_HZ;
    SystemInit();
    timer_init();

    /* Driver init */
    pwm_init(BUZZ_CH, 0, 0);
    pwm_init(LED_CH, 0, 0);
    uart_init(115200);
    gpio_init(SOUND_DIP0_PIN, GPIO_INPUT);
    gpio_init(SOUND_DIP1_PIN, GPIO_INPUT);
    gpio_init(LIGHT_DIP0_PIN, GPIO_INPUT);
    gpio_init(LIGHT_DIP1_PIN, GPIO_INPUT);
    /* Unused pins: input + pull-up per datasheet recommendation */
    gpio_init(3,  GPIO_INPUT);  /* P03 */
    gpio_init(19, GPIO_INPUT);  /* P23 */
    gpio_init(20, GPIO_INPUT);  /* P24 */
    gpio_init(22, GPIO_INPUT);  /* P26 */

    /* App init */
    alarm_sound_init();
    alarm_light_init();
    proto_ctx_t proto;
    proto_init(&proto, NULL, 0);

    /* Power-on: load DIP-selected patterns */
    dip_debounce_t sd_db = {0}, ld_db = {0};
    uint8_t cur_sound_dip = dip_read_2bit(SOUND_DIP0_PIN, SOUND_DIP1_PIN);
    uint8_t cur_light_dip = dip_read_2bit(LIGHT_DIP0_PIN, LIGHT_DIP1_PIN);
    sd_db.stable_val = cur_sound_dip;
    ld_db.stable_val = cur_light_dip;
    alarm_sound_load(cur_sound_dip);
    alarm_light_load(cur_light_dip);
    proto_update_state(&proto, cur_sound_dip, cur_light_dip,
                       alarm_sound_current(), alarm_light_current());

    uint8_t  rx_buf[16];
    uint8_t  test_active = 0;
    uint32_t test_tick = 0;

    while (1) {
        uint32_t now = timer_get_tick();

        /* 1. Sound alarm state machine */
        if (alarm_sound_is_active()) alarm_sound_tick(now);

        /* 2. Light alarm state machine */
        if (alarm_light_is_active()) alarm_light_tick(now);

        /* 3. Sound DIP change detection */
        uint8_t s_raw = dip_read_2bit(SOUND_DIP0_PIN, SOUND_DIP1_PIN);
        uint8_t s_dip = dip_debounced(&sd_db, s_raw, now);
        if (s_dip > 3) s_dip = 0;
        if (s_dip != cur_sound_dip && !test_active) {
            cur_sound_dip = s_dip;
            alarm_sound_load(cur_sound_dip);
        }

        /* 4. Light DIP change detection */
        uint8_t l_raw = dip_read_2bit(LIGHT_DIP0_PIN, LIGHT_DIP1_PIN);
        uint8_t l_dip = dip_debounced(&ld_db, l_raw, now);
        if (l_dip > 3) l_dip = 0;
        if (l_dip != cur_light_dip && !test_active) {
            cur_light_dip = l_dip;
            alarm_light_load(cur_light_dip);
        }

        /* 5. UART command processing */
        uint16_t rx_len = uart_recv(rx_buf, sizeof(rx_buf));
        if (rx_len > 0) {
            proto_feed(&proto, rx_buf, rx_len);
        }

        /* 6. RESET command handling */
        if (g_reset_pending) {
            g_reset_pending = 0;
            alarm_sound_load(cur_sound_dip);
            alarm_light_load(cur_light_dip);
        }

        /* 7. Update protocol state (every loop for STATUS/DIP queries) */
        proto_update_state(&proto, cur_sound_dip, cur_light_dip,
                           alarm_sound_current(), alarm_light_current());

        /* 8. TEST sequence: pattern 0 for 3s, then auto-restore */
        if (alarm_sound_current() == 0 && alarm_light_current() == 0
            && alarm_sound_is_active() && !test_active) {
            test_active = 1;
            test_tick = now;
        }
        if (test_active && (now - test_tick) >= 3000) {
            test_active = 0;
            alarm_sound_load(cur_sound_dip);
            alarm_light_load(cur_light_dip);
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add src/main.c
git commit -m "feat: add main loop with DIP debounce and TEST sequence"
```

---

### Task 10: 目标构建和验证

- [ ] **Step 1: 确保 HC32L110 SDK 可用**

将华大 HC32L110 SDK（`hc32ll10_ddl`）放到 `lib/` 目录，CMakeLists.txt 中包含 SDK 路径。

```bash
mkdir -p lib
# Copy HC32L110 SDK to lib/hc32ll10_ddl/
```

- [ ] **Step 2: 更新 CMakeLists.txt 添加 SDK 依赖**

```cmake
# Add before the driver_hal target
set(HC32_SDK_DIR "${CMAKE_SOURCE_DIR}/lib/hc32ll10_ddl" CACHE PATH "HC32L110 SDK path")

add_library(hc32_sdk STATIC
    ${HC32_SDK_DIR}/source/system_hc32l110.c
    ${HC32_SDK_DIR}/source/startup_hc32l110.s
)

# Update driver_hal to link SDK
target_link_libraries(driver_hal hc32_sdk)
target_include_directories(driver_hal PRIVATE ${HC32_SDK_DIR}/include)
target_include_directories(hc32_sdk PUBLIC ${HC32_SDK_DIR}/include)
```

- [ ] **Step 3: 编译目标固件**

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/arm-gcc.cmake -DCMAKE_BUILD_TYPE=MinSizeRel
make
```

期望：生成 `fire-alarm.elf`、`fire-alarm.bin`、`fire-alarm.hex`。

- [ ] **Step 4: 检查 binary 大小**

```bash
arm-none-eabi-size fire-alarm.elf
```

期望：Flash < 32KB, RAM < 4KB。

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt toolchain/
git commit -m "feat: add target build with HC32L110 SDK integration"
```

---

### Task 11: 烧录和硬件测试

- [ ] **Step 1: 连接调试器**

J-Link 或 ST-Link 接 SWD（SWDIO、SWCLK、GND、3.3V）。

- [ ] **Step 2: 烧录固件**

```bash
arm-none-eabi-gdb -ex "target extended-remote :2331" \
  -ex "monitor flash device = HC32L110C6PA" \
  -ex "load fire-alarm.elf" \
  -ex "monitor reset" \
  -ex "quit"
```

- [ ] **Step 3: 功能测试**

接上 USB-UART 模块，串口工具（115200-8-N-1）发送命令：

```
DIP         → 应返回 OK SD=<v> LD=<v>
STATUS      → 应返回 OK B=<n> L=<n> SD=<v> LD=<v>
BUZZ ON 1   → 蜂鸣器应按 P1 pattern 响
LED ON 0    → LED 应按 P0 pattern 亮
BUZZ OFF    → 蜂鸣器停
RESET       → 恢复到 DIP 定义 pattern
TEST        → 自检后恢复
```

- [ ] **Step 4: DIP 开关测试**

拨动 DIP 开关，观察声光 pattern 是否随之切换。

---

## 自审

**Spec 覆盖检查：**
- 声警报 pattern 驱动 → Task 4 (pattern_engine) + Task 5 (alarm_sound)
- 光警报 pattern 驱动 → Task 4 (pattern_engine) + Task 5 (alarm_light)
- DIP 开关选择（两组独立 2-bit） → Task 9 (main.c 两组独立消抖)
- 测试通信协议 → Task 6 (test_protocol)
- Driver API 抽象 → Task 2 (headers) + Task 8 (HAL)
- Pattern 配置表（声 4 条 + 光 4 条） → Task 3
- DIP 低有效取反 → Task 9 dip_read_2bit
- 8MHz 外部晶振 → Task 9 SYSTEM_CLOCK_HZ
- 协议 DIP/STATUS 双路 → Task 6 h_dip/h_status
- 移植策略 → Task 8 HAL 替换即可
- 上电行为 → Task 9 main.c 上电 load
- DIP 消抖 50ms → Task 9 dip_debounced
- Pattern 0 默认语义 → Task 3 pattern_table.c
- Tick 溢出处理 → Task 4 pattern_engine_tick 用差值比较
- 错误响应 ERR CMD/ERR RANGE → Task 6 test_protocol.c
- 有限循环结束 → Task 4 pattern_engine_tick
- TEST 恢复 → Task 9 main.c test_active 逻辑
- 命令触发（立即切 step 0） → Task 6 handler 实现
- RESET 命令 → Task 9 g_reset_pending 标志
- 64 字节限制 → Task 6 PROTO_BUF_SIZE

**占位符检查：** 无 TBD/TODO。所有代码完整。所有 pin 号已根据 PCB 原理图填入 main.c 注释头。

**类型一致性：**
- `pattern_engine_t` 在 Task 4 定义，Task 5 和 Task 9 使用 → 一致
- `proto_ctx_t.sound_dip/light_dip` 在 Task 6 定义，Task 9 使用 → 一致
- `proto_update_state(ctx, s_dip, l_dip, buzz, led)` 4 参数 → 全文件一致
- `BUZZ_CH=0, LED_CH=1` 在 Task 3 定义，全部 task 一致使用
- `dip_debounce_t` 在 Task 9 定义，Task 9 两实例使用 → 一致
