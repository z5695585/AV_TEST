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
