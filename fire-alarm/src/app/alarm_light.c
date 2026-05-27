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

void alarm_light_set_cmd_mode(void) {
    pattern_engine_set_cmd_mode(&light_engine);
}

void alarm_light_clear_cmd_mode(void) {
    pattern_engine_clear_cmd_mode(&light_engine);
}

uint8_t alarm_light_in_cmd_mode(void) {
    return pattern_engine_in_cmd_mode(&light_engine);
}

void alarm_light_set_freq_ovr(uint32_t hz) {
    pattern_engine_set_freq_ovr(&light_engine, hz);
}

void alarm_light_set_duty_ovr(uint8_t pct) {
    pattern_engine_set_duty_ovr(&light_engine, pct);
}

void alarm_light_clear_overrides(void) {
    pattern_engine_clear_overrides(&light_engine);
}
