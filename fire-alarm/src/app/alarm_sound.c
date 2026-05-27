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

void alarm_sound_set_cmd_mode(void) {
    pattern_engine_set_cmd_mode(&sound_engine);
}

void alarm_sound_clear_cmd_mode(void) {
    pattern_engine_clear_cmd_mode(&sound_engine);
}

uint8_t alarm_sound_in_cmd_mode(void) {
    return pattern_engine_in_cmd_mode(&sound_engine);
}

void alarm_sound_set_freq_ovr(uint32_t hz) {
    pattern_engine_set_freq_ovr(&sound_engine, hz);
}

void alarm_sound_set_duty_ovr(uint8_t pct) {
    pattern_engine_set_duty_ovr(&sound_engine, pct);
}

void alarm_sound_clear_overrides(void) {
    pattern_engine_clear_overrides(&sound_engine);
}
