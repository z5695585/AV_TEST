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
    uint8_t  cmd_mode;          /* 1 = command override, ignore DIP */
    uint8_t  freq_ovr_active;   /* 1 = override freq */
    uint8_t  duty_ovr_active;   /* 1 = override duty */
    uint8_t  duty_ovr_val;      /* 5-100 */
    uint32_t freq_ovr_val;      /* 2000-5000 */
} pattern_engine_t;

void     pattern_engine_init(pattern_engine_t *ctx, uint8_t channel);
uint8_t  pattern_engine_load(pattern_engine_t *ctx, uint8_t pattern_idx);
void     pattern_engine_tick(pattern_engine_t *ctx, uint32_t now);
uint8_t  pattern_engine_is_active(const pattern_engine_t *ctx);
void     pattern_engine_stop(pattern_engine_t *ctx);
uint8_t  pattern_engine_current_index(const pattern_engine_t *ctx);

/* Command override API */
void     pattern_engine_set_cmd_mode(pattern_engine_t *ctx);
void     pattern_engine_clear_cmd_mode(pattern_engine_t *ctx);
uint8_t  pattern_engine_in_cmd_mode(const pattern_engine_t *ctx);
void     pattern_engine_set_freq_ovr(pattern_engine_t *ctx, uint32_t hz);
void     pattern_engine_set_duty_ovr(pattern_engine_t *ctx, uint8_t pct);
void     pattern_engine_clear_overrides(pattern_engine_t *ctx);

#endif
