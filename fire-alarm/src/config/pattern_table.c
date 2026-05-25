#include "pattern_table.h"

/* ================================================================
 * 声警报 Pattern 表（4 条，由 2-bit DIP pin19/20 选择）
 *
 * T3 交替音：6-step 序列模拟 ISO 8201 T3 模式
 * 连续音：单 step 持续输出
 * 高档 = 50% 占空比，低档 = 25% 占空比
 * ================================================================ */

/* P0: T3 交替，低档（25%） */
static const pattern_step_t sound_p0_steps[] = {
    {3000, 25, 500},
    {0,    0,  500},
    {3000, 25, 500},
    {0,    0,  500},
    {3000, 25, 500},
    {0,    0, 1500},
};

/* P1: T3 交替，高档（50%） */
static const pattern_step_t sound_p1_steps[] = {
    {3000, 50, 500},
    {0,    0,  500},
    {3000, 50, 500},
    {0,    0,  500},
    {3000, 50, 500},
    {0,    0, 1500},
};

/* P2: 连续音，低档（25%） */
static const pattern_step_t sound_p2_steps[] = {
    {3000, 25, 0},
};

/* P3: 连续音，高档（50%） */
static const pattern_step_t sound_p3_steps[] = {
    {3000, 50, 0},
};

const pattern_t sound_pattern_table[MAX_PATTERNS] = {
    {6, sound_p0_steps, 0},  /* P0: T3 low */
    {6, sound_p1_steps, 0},  /* P1: T3 high */
    {1, sound_p2_steps, 0},  /* P2: continuous low */
    {1, sound_p3_steps, 0},  /* P3: continuous high */
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
};

/* ================================================================
 * 光警报 Pattern 表（4 条，由 2-bit DIP pin2/3 选择）
 *
 * 时序：20ms on / 980ms off 循环
 * 亮度通过 on 期间的 PWM 占空比区分
 * ================================================================ */

/* P0: 低亮度（on 期间 10% duty） */
static const pattern_step_t light_p0_steps[] = {
    {3000, 10,  20},
    {0,    0,  980},
};

/* P1: 中亮度（on 期间 50% duty） */
static const pattern_step_t light_p1_steps[] = {
    {3000, 50,  20},
    {0,    0,  980},
};

/* P2: 高亮度（on 期间 100% duty — 20ms 全高） */
static const pattern_step_t light_p2_steps[] = {
    {3000, 100, 20},
    {0,    0,  980},
};

/* P3: 预留，回退到 P0 */

const pattern_t light_pattern_table[MAX_PATTERNS] = {
    {2, light_p0_steps, 0},  /* P0: 10% */
    {2, light_p1_steps, 0},  /* P1: 50% */
    {2, light_p2_steps, 0},  /* P2: 100% */
    {0, NULL,          0},  /* P3: reserved → fallback */
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
    {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0}, {0, NULL, 0},
};
