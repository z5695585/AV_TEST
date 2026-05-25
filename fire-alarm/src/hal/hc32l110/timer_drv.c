/* timer_drv.c — HC32L110 HAL: SysTick timer (1ms tick)
 *
 * Uses CMSIS SysTick_Config() from core_cm0plus.h.
 * SystemCoreClock = 8MHz (external XTH crystal).
 */
#include "timer_drv.h"
#include "ddl.h"

static volatile uint32_t s_tick_ms = 0;

void SysTick_Handler(void) {
    s_tick_ms++;
}

void timer_init(void) {
    Clk_SwitchTo(ClkXTH);
    SystemCoreClock = 8000000U;
    SysTick_Config(SystemCoreClock / 1000);
}

void timer_delay_ms(uint32_t ms) {
    uint32_t start = s_tick_ms;
    while ((s_tick_ms - start) < ms) { /* wait */ }
}

void timer_delay_us(uint32_t us) {
    uint32_t count = us * (SystemCoreClock / 4000000);
    while (count--) { __NOP(); }
}

uint32_t timer_get_tick(void) {
    return s_tick_ms;
}
