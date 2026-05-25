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
