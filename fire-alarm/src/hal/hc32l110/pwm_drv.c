/* pwm_drv.c — HC32L110 HAL: PWM via ADT (TIM4_CHA + TIM5_CHA)
 *
 * Channel 0 (Buzzer): TIM4 (ADT0) CHA → P34 (pin 1)
 * Channel 1 (LED):    TIM5 (ADT1) CHA → P25 (pin 15)
 */
#include "pwm_drv.h"
#include "ddl.h"
#include "adt.h"
#include "gpio.h"

static uint16_t s_period[2];

void pwm_init(uint8_t channel, uint32_t freq_hz, uint8_t duty_pct) {
    en_adt_unit_t            enAdt = (channel == 0) ? AdTIM4 : AdTIM5;
    stc_adt_basecnt_cfg_t    stcCfg;
    stc_adt_CHxX_port_cfg_t  stcPortCfg;
    uint16_t period, compare;

    if (freq_hz == 0) return;

    Clk_SetPeripheralGate(ClkPeripheralGpio, TRUE);
    Clk_SetPeripheralGate(ClkPeripheralAdt, TRUE);

    if (channel == 0) {
        Gpio_SetFunc_TIM4_CHA_P34();
    } else {
        Gpio_SetFunc_TIM5_CHA_P25();
    }

    /* Timer base configuration */
    DDL_ZERO_STRUCT(stcCfg);
    stcCfg.enCntMode   = AdtSawtoothMode;
    stcCfg.enCntDir    = AdtCntUp;
    stcCfg.enCntClkDiv = AdtClkPClk0;

    Adt_StopCount(enAdt);
    Adt_ClearCount(enAdt);
    Adt_Init(enAdt, &stcCfg);

    period  = (uint16_t)(SystemCoreClock / freq_hz);
    compare = (uint16_t)((uint32_t)period * duty_pct / 100);
    s_period[channel] = period;

    Adt_SetPeriod(enAdt, period);
    Adt_SetCompareValue(enAdt, AdtCompareA, compare);

    /* Port output configuration — REQUIRED to drive the physical pin */
    DDL_ZERO_STRUCT(stcPortCfg);
    stcPortCfg.enCap    = AdtCHxCompareOutput;
    stcPortCfg.bOutEn   = TRUE;
    stcPortCfg.enPerc   = AdtCHxPeriodHigh;
    stcPortCfg.enCmpc   = AdtCHxCompareLow;
    stcPortCfg.enStaStp = AdtCHxStateSelSS;
    stcPortCfg.enStaOut = AdtCHxPortOutHigh;
    stcPortCfg.enStpOut = AdtCHxPortOutLow;
    Adt_CHxXPortConfig(enAdt, AdtCHxA, &stcPortCfg);

    Adt_StartCount(enAdt);
}

void pwm_set_freq(uint8_t channel, uint32_t freq_hz) {
    en_adt_unit_t enAdt;
    uint16_t period;
    if (freq_hz == 0) return;
    enAdt  = (channel == 0) ? AdTIM4 : AdTIM5;
    period = (uint16_t)(SystemCoreClock / freq_hz);
    s_period[channel] = period;
    Adt_SetPeriod(enAdt, period);
}

void pwm_set_duty(uint8_t channel, uint8_t duty_pct) {
    en_adt_unit_t enAdt  = (channel == 0) ? AdTIM4 : AdTIM5;
    uint16_t compare = (uint16_t)((uint32_t)s_period[channel] * duty_pct / 100);
    Adt_SetCompareValue(enAdt, AdtCompareA, compare);
}

void pwm_start(uint8_t channel) {
    en_adt_unit_t enAdt = (channel == 0) ? AdTIM4 : AdTIM5;
    Adt_StartCount(enAdt);
}

void pwm_stop(uint8_t channel) {
    en_adt_unit_t enAdt = (channel == 0) ? AdTIM4 : AdTIM5;
    Adt_StopCount(enAdt);
}
