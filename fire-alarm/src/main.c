/* src/main.c
 *
 * Pin mapping (HC32L110C6PA, TSSOP-20, Rev2.6 datasheet):
 *   Physical pin → GPIO name (logical#) : Function
 *   Pin 1  → P34 (28) : Buzzer PWM — TIM4_CHA, open-drain → transistor drive
 *   Pin 15 → P25 (21) : LED PWM   — TIM5_CHA, → LED driver IC input
 *   Pin 12 → P14 (12) : UART0 TX
 *   Pin 11 → P15 (13) : UART0 RX
 *   Pin 19 → P32 (26) : Sound DIP BIT0 (LSB)
 *   Pin 20 → P33 (27) : Sound DIP BIT1 (MSB)
 *   Pin 2  → P35 (29) : Light DIP BIT0 (LSB)
 *   Pin 3  → P36 (30) : Light DIP BIT1 (MSB)
 *   Pin 5  → P01 (1)  : XTHI (8MHz crystal)
 *   Pin 6  → P02 (2)  : XTHO (8MHz crystal)
 *   Pin 9  → AVCC/DVCC : VDD 3.3V
 *   Pin 7  → AVSS/DVSS : GND
 *   Pin 4  → P00 (0)  : RESETB (NRST, active low)
 *   Pin 17 → P27 (23) : SWDIO
 *   Pin 18 → P31 (25) : SWCLK
 *   Unused: P03(3), P23(19), P24(20), P26(22) → input + pull-up
 *
 * GPIO logical# = Port×8 + Bit. P00=0, P14=12, P23=19, P31=25, etc.
 * DIP: common = GND → active LOW → software inverts so 1 = ON.
 */
#include "ddl.h"
#include "alarm_sound.h"
#include "alarm_light.h"
#include "test_protocol.h"
#include "gpio_drv.h"
#include "uart_drv.h"
#include "timer_drv.h"
#include "pwm_drv.h"
#include "pattern_table.h"

/* --- Pin assignments (GPIO logical numbers = Port×8 + Bit) --- */
#define SOUND_DIP0_PIN  26   /* P32 */
#define SOUND_DIP1_PIN  27   /* P33 */
#define LIGHT_DIP0_PIN  29   /* P35 */
#define LIGHT_DIP1_PIN  30   /* P36 */

/* System clock: 8MHz external crystal, no PLL (low-power preference) */
#define SYSTEM_CLOCK_HZ  8000000U

/* --- Command handler callbacks --- */
void proto_buzz_load(uint8_t idx) { alarm_sound_load(idx); }
void proto_buzz_stop(void)         { alarm_sound_stop(); }
void proto_led_load(uint8_t idx)   { alarm_light_load(idx); }
void proto_led_stop(void)          { alarm_light_stop(); }

static volatile uint8_t g_reset_pending = 0;
void proto_reset_to_dip(void) { g_reset_pending = 1; }

/* --- DIP reading (2-bit, active LOW → invert) --- */

static uint8_t dip_read_2bit(uint8_t pin0, uint8_t pin1) {
    uint8_t raw = 0;
    if (gpio_read(pin0)) raw |= 1;
    if (gpio_read(pin1)) raw |= 2;
    return raw ^ 0x03;  /* invert: common=GND → LOW=ON → 1=ON */
}

typedef struct {
    uint8_t  stable_val;
    uint8_t  last_raw;
    uint32_t last_change_tick;
} dip_debounce_t;

static uint8_t dip_debounced(dip_debounce_t *d, uint8_t raw, uint32_t now) {
    if (raw != d->last_raw) {
        d->last_raw = raw;
        d->last_change_tick = now;
    }
    if ((now - d->last_change_tick) >= 50) {
        d->stable_val = raw;
    }
    return d->stable_val;
}

/* --- main --- */
int main(void) {
    SystemCoreClock = SYSTEM_CLOCK_HZ;
    SystemInit();
    timer_init();

    Clk_SetPeripheralGate(ClkPeripheralGpio, TRUE);

    /* Init PWM with default freq=3kHz, duty=0 (no output until pattern loads) */
    pwm_init(BUZZ_CH, 3000, 0);
    pwm_init(LED_CH, 3000, 0);
    uart_init(9600);

    gpio_init(SOUND_DIP0_PIN, GPIO_INPUT);
    gpio_init(SOUND_DIP1_PIN, GPIO_INPUT);
    gpio_init(LIGHT_DIP0_PIN, GPIO_INPUT);
    gpio_init(LIGHT_DIP1_PIN, GPIO_INPUT);
    gpio_init(3,  GPIO_INPUT);  /* P03: unused */
    gpio_init(19, GPIO_INPUT);  /* P23: unused */
    gpio_init(20, GPIO_INPUT);  /* P24: unused */
    gpio_init(22, GPIO_OUTPUT); /* P26: heartbeat debug output */

    alarm_sound_init();
    alarm_light_init();
    proto_ctx_t proto;
    proto_init(&proto, NULL, 0);

    /* Power-on: read DIP and load patterns */
    dip_debounce_t sd_db = {0}, ld_db = {0};
    uint8_t cur_sound_dip = dip_read_2bit(SOUND_DIP0_PIN, SOUND_DIP1_PIN);
    uint8_t cur_light_dip = dip_read_2bit(LIGHT_DIP0_PIN, LIGHT_DIP1_PIN);
    sd_db.stable_val = cur_sound_dip;
    ld_db.stable_val = cur_light_dip;
    alarm_sound_load(cur_sound_dip);
    alarm_light_load(cur_light_dip);
    proto_update_state(&proto, cur_sound_dip, cur_light_dip,
                       alarm_sound_current(), alarm_light_current());

    uart_send_str("\r\n=== Fire Alarm v1.0 ===\r\n");
    uart_send_str("UART 9600-8-N-1 ready. Type HELP for commands.\r\n\r\n");

    uint8_t  rx_buf[16];
    uint8_t  heartbeat = 0;
    uint32_t hb_tick = 0;

    while (1) {
        uint32_t now = timer_get_tick();

        /* Heartbeat: toggle P26 every 500ms → 1Hz square wave */
        if ((now - hb_tick) >= 500) {
            hb_tick = now;
            heartbeat = !heartbeat;
            gpio_write(22, heartbeat);
        }

        if (alarm_sound_is_active()) alarm_sound_tick(now);
        if (alarm_light_is_active()) alarm_light_tick(now);

        /* Sound DIP change detection (2-bit, pin 19/20, active LOW inverted) */
        uint8_t s_raw = dip_read_2bit(SOUND_DIP0_PIN, SOUND_DIP1_PIN);
        uint8_t s_dip = dip_debounced(&sd_db, s_raw, now);
        if (s_dip > 3) s_dip = 0;
        if (s_dip != cur_sound_dip) {
            cur_sound_dip = s_dip;
            alarm_sound_load(cur_sound_dip);
        }

        /* Light DIP change detection (2-bit, pin 2/3, active LOW inverted) */
        uint8_t l_raw = dip_read_2bit(LIGHT_DIP0_PIN, LIGHT_DIP1_PIN);
        uint8_t l_dip = dip_debounced(&ld_db, l_raw, now);
        if (l_dip > 3) l_dip = 0;
        if (l_dip != cur_light_dip) {
            cur_light_dip = l_dip;
            alarm_light_load(cur_light_dip);
        }

        /* UART disabled for debug */
        /* uint16_t rx_len = uart_recv(rx_buf, sizeof(rx_buf)); */
        /* if (rx_len > 0) { */
        /*     proto_feed(&proto, rx_buf, rx_len); */
        /* } */

        if (g_reset_pending) {
            g_reset_pending = 0;
            alarm_sound_load(cur_sound_dip);
            alarm_light_load(cur_light_dip);
        }

        proto_update_state(&proto, cur_sound_dip, cur_light_dip,
                           alarm_sound_current(), alarm_light_current());
    }
}
