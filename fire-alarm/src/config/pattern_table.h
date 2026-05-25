#ifndef PATTERN_TABLE_H
#define PATTERN_TABLE_H
#include <stdint.h>
#include <stddef.h>

#define MAX_PATTERNS    16
#define BUZZ_CH         0
#define LED_CH          1

typedef struct {
    uint32_t freq_hz;
    uint8_t  duty_pct;
    uint16_t duration_ms;
} pattern_step_t;

typedef struct {
    uint8_t               step_count;
    const pattern_step_t *steps;
    uint8_t               loop_count;  /* 0=infinite */
} pattern_t;

extern const pattern_t sound_pattern_table[MAX_PATTERNS];
extern const pattern_t light_pattern_table[MAX_PATTERNS];

#endif
