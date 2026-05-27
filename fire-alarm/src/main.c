/* src/main.c — Fire Alarm Audio-Visual Alarm Firmware
 *
 * HC32L110C6PA, TSSOP-20. 8MHz XTH. UART0 9600-8-N-1 on P14/P15.
 */
#include "ddl.h"
#include "uart.h"
#include "alarm_sound.h"
#include "alarm_light.h"
#include "test_protocol.h"
#include "gpio_drv.h"
#include "uart_drv.h"
#include "timer_drv.h"
#include "pwm_drv.h"
#include "pattern_table.h"

#define SOUND_DIP0_PIN  26   /* P32 */
#define SOUND_DIP1_PIN  27   /* P33 */
#define LIGHT_DIP0_PIN  29   /* P35 */
#define LIGHT_DIP1_PIN  30   /* P36 */

#define SYSTEM_CLOCK_HZ  8000000U

void proto_buzz_load(uint8_t idx) { alarm_sound_load(idx); }
void proto_buzz_stop(void)         { alarm_sound_stop(); }
void proto_led_load(uint8_t idx)   { alarm_light_load(idx); }
void proto_led_stop(void)          { alarm_light_stop(); }

static volatile uint8_t g_reset_pending = 0;
void proto_reset_to_dip(void) { g_reset_pending = 1; }

static uint8_t dip_read_2bit(uint8_t pin0, uint8_t pin1) {
    uint8_t raw = 0;
    if (gpio_read(pin0)) raw |= 1;
    if (gpio_read(pin1)) raw |= 2;
    return raw ^ 0x03;
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

int main(void) {
    SystemCoreClock = SYSTEM_CLOCK_HZ;
    SystemInit();
    timer_init();

    Clk_SetPeripheralGate(ClkPeripheralGpio, TRUE);

    pwm_init(BUZZ_CH, 3000, 0);
    pwm_init(LED_CH, 3000, 0);
    uart_init(9600);

    gpio_init(SOUND_DIP0_PIN, GPIO_INPUT);
    gpio_init(SOUND_DIP1_PIN, GPIO_INPUT);
    gpio_init(LIGHT_DIP0_PIN, GPIO_INPUT);
    gpio_init(LIGHT_DIP1_PIN, GPIO_INPUT);
    gpio_init(3,  GPIO_INPUT);
    gpio_init(19, GPIO_INPUT);
    gpio_init(20, GPIO_INPUT);
    gpio_init(22, GPIO_OUTPUT);

    alarm_sound_init();
    alarm_light_init();
    proto_ctx_t proto;
    proto_init(&proto, NULL, 0);

    dip_debounce_t sd_db = {0}, ld_db = {0};
    uint8_t cur_sound_dip = dip_read_2bit(SOUND_DIP0_PIN, SOUND_DIP1_PIN);
    uint8_t cur_light_dip = dip_read_2bit(LIGHT_DIP0_PIN, LIGHT_DIP1_PIN);
    sd_db.stable_val = cur_sound_dip;
    ld_db.stable_val = cur_light_dip;
    alarm_sound_load(cur_sound_dip);
    alarm_light_load(cur_light_dip);
    proto_update_state(&proto, cur_sound_dip, cur_light_dip,
                       alarm_sound_current(), alarm_light_current());

    uint8_t  rx_buf[64];
    uint8_t  heartbeat = 0;
    uint32_t hb_tick   = 0;
    uint8_t  banner_sent = 0;

    while (1) {
        uint32_t now = timer_get_tick();

        /* Banner: send once, 4 seconds after power-on */
        if (!banner_sent && now >= 4000) {
            banner_sent = 1;
            uart_send_str("\r\n"
                          "========================================\r\n"
                          "  Fire Alarm Audio-Visual Alert v0.1\r\n"
                          "  HC32L110C6PA | 8MHz XTH | UART 9600-8-N-1\r\n"
                          "========================================\r\n"
                          "COMMANDS:\r\n"
                          "  PING           - Test connection, returns PONG\r\n"
                          "  HELP           - Show this command list\r\n"
                          "  DIP            - Read DIP switch positions\r\n"
                          "                   (Sound:2bit Light:2bit)\r\n"
                          "  STATUS         - Show current pattern indices\r\n"
                          "                   and DIP states\r\n"
                          "  BUZZ ON <n>    - Start buzzer pattern n (0-3)\r\n"
                          "  BUZZ OFF       - Stop buzzer immediately\r\n"
                          "  LED ON <n>     - Start LED pattern n (0-3)\r\n"
                          "  LED OFF        - Stop LED immediately\r\n"
                          "  RESET          - Reload patterns from DIP\r\n"
                          "  TEST           - Load both pattern 0\r\n"
                          "========================================\r\n"
                          "SOUND PATTERNS (by DIP / BUZZ ON n):\r\n"
                          "  0: T3 tone 25% duty | 1: T3 tone 50% duty\r\n"
                          "  2: Continuous 25%   | 3: Continuous 50%\r\n"
                          "LIGHT PATTERNS (by DIP / LED ON n):\r\n"
                          "  0: 20ms flash 10%   | 1: 20ms flash 50%\r\n"
                          "  2: 20ms flash 100%  | 3: fallback 10%\r\n"
                          "========================================\r\n\r\n");
        }

        /* Heartbeat: toggle P26 every 500ms */
        if ((now - hb_tick) >= 500) {
            hb_tick = now;
            heartbeat = !heartbeat;
            gpio_write(22, heartbeat);
        }

        if (alarm_sound_is_active()) alarm_sound_tick(now);
        if (alarm_light_is_active()) alarm_light_tick(now);

        /* Sound DIP */
        uint8_t s_raw = dip_read_2bit(SOUND_DIP0_PIN, SOUND_DIP1_PIN);
        uint8_t s_dip = dip_debounced(&sd_db, s_raw, now);
        if (s_dip > 3) s_dip = 0;
        if (s_dip != cur_sound_dip) {
            cur_sound_dip = s_dip;
            alarm_sound_load(cur_sound_dip);
        }

        /* Light DIP */
        uint8_t l_raw = dip_read_2bit(LIGHT_DIP0_PIN, LIGHT_DIP1_PIN);
        uint8_t l_dip = dip_debounced(&ld_db, l_raw, now);
        if (l_dip > 3) l_dip = 0;
        if (l_dip != cur_light_dip) {
            cur_light_dip = l_dip;
            alarm_light_load(cur_light_dip);
        }

        /* UART command processing */
        uint16_t rx_len = uart_recv(rx_buf, sizeof(rx_buf));
        if (rx_len > 0) {
            proto_feed(&proto, rx_buf, rx_len);
        }

        if (g_reset_pending) {
            g_reset_pending = 0;
            alarm_sound_load(cur_sound_dip);
            alarm_light_load(cur_light_dip);
        }

        proto_update_state(&proto, cur_sound_dip, cur_light_dip,
                           alarm_sound_current(), alarm_light_current());
    }
}
