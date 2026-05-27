#include "pattern_engine.h"
#include "pattern_table.h"
#include "pwm_drv.h"
#include <stddef.h>

static const pattern_t *lookup_pattern(const pattern_t *table, uint8_t idx) {
    if (idx >= MAX_PATTERNS) idx = 0;
    const pattern_t *p = &table[idx];
    if (p->step_count == 0 || p->steps == NULL) {
        p = &table[0];
        if (p->step_count == 0 || p->steps == NULL) return NULL;
    }
    return p;
}

/* Apply freq/duty, respecting overrides.
 * Step freq==0 means "off" — always respected, overrides only apply in "on" steps.
 * Uses pwm_reconfig (stop→clear→set both→start) for clean first cycle. */
static void apply_step_params(pattern_engine_t *ctx, const pattern_step_t *step) {
    if (step->freq_hz == 0) {
        pwm_stop(ctx->pwm_channel);
    } else {
        uint32_t freq = ctx->freq_ovr_active ? ctx->freq_ovr_val : step->freq_hz;
        uint8_t  duty = ctx->duty_ovr_active ? ctx->duty_ovr_val : step->duty_pct;
        pwm_reconfig(ctx->pwm_channel, freq, duty);
    }
}

void pattern_engine_init(pattern_engine_t *ctx, uint8_t channel) {
    ctx->pattern        = NULL;
    ctx->cur_step       = 0;
    ctx->cur_loop       = 0;
    ctx->step_start_tick = 0;
    ctx->active         = 0;
    ctx->pwm_channel    = channel;
    ctx->cmd_mode       = 0;
    ctx->freq_ovr_active = 0;
    ctx->duty_ovr_active = 0;
    ctx->freq_ovr_val   = 0;
    ctx->duty_ovr_val   = 0;
}

uint8_t pattern_engine_load(pattern_engine_t *ctx, uint8_t pattern_idx) {
    const pattern_t *table = (ctx->pwm_channel == BUZZ_CH)
        ? sound_pattern_table : light_pattern_table;
    const pattern_t *p = lookup_pattern(table, pattern_idx);
    if (!p) return 0;

    ctx->pattern        = p;
    ctx->cur_step       = 0;
    ctx->cur_loop       = 0;
    ctx->step_start_tick = 0;
    ctx->active         = 1;

    apply_step_params(ctx, &p->steps[0]);
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
        apply_step_params(ctx, step);
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

/* --- Command override API --- */

void pattern_engine_set_cmd_mode(pattern_engine_t *ctx) {
    ctx->cmd_mode = 1;
}

void pattern_engine_clear_cmd_mode(pattern_engine_t *ctx) {
    ctx->cmd_mode = 0;
}

uint8_t pattern_engine_in_cmd_mode(const pattern_engine_t *ctx) {
    return ctx->cmd_mode;
}

void pattern_engine_set_freq_ovr(pattern_engine_t *ctx, uint32_t hz) {
    ctx->freq_ovr_active = 1;
    ctx->freq_ovr_val = hz;
    if (ctx->active) {
        uint8_t duty = ctx->duty_ovr_active ? ctx->duty_ovr_val
                      : ctx->pattern->steps[ctx->cur_step].duty_pct;
        pwm_reconfig(ctx->pwm_channel, hz, duty);
    }
}

void pattern_engine_set_duty_ovr(pattern_engine_t *ctx, uint8_t pct) {
    ctx->duty_ovr_active = 1;
    ctx->duty_ovr_val = pct;
    if (ctx->active) {
        uint32_t freq = ctx->freq_ovr_active ? ctx->freq_ovr_val
                       : ctx->pattern->steps[ctx->cur_step].freq_hz;
        pwm_reconfig(ctx->pwm_channel, freq, pct);
    }
}

void pattern_engine_clear_overrides(pattern_engine_t *ctx) {
    ctx->cmd_mode       = 0;
    ctx->freq_ovr_active = 0;
    ctx->duty_ovr_active = 0;
}
