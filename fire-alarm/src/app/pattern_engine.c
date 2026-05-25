#include "pattern_engine.h"
#include "pattern_table.h"
#include "pwm_drv.h"
#include <stddef.h>

static const pattern_t *lookup_pattern(const pattern_t *table, uint8_t idx) {
    if (idx >= MAX_PATTERNS) idx = 0;  /* clamp, per spec: out-of-range → P0 */
    const pattern_t *p = &table[idx];
    if (p->step_count == 0 || p->steps == NULL) {
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
    if (!ctx->active || !ctx->pattern) return 0xFF;
    const pattern_t *table = (ctx->pwm_channel == BUZZ_CH)
        ? sound_pattern_table : light_pattern_table;
    for (uint8_t i = 0; i < MAX_PATTERNS; i++) {
        if (&table[i] == ctx->pattern) return i;
    }
    return 0xFF;
}
