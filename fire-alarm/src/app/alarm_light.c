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
