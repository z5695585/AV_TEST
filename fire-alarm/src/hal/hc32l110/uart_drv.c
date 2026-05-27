/* uart_drv.c — HC32L110 HAL: UART0 (P14/TX, P15/RX)
 *
 * Direct register access, following SDK Debug_UartInit / Debug_Output pattern.
 * UART0 baud rate clock: BT0 toggle output.
 */
#include "uart_drv.h"
#include "ddl.h"
#include "uart.h"
#include "gpio.h"

void uart_init(uint32_t baudrate) {
    uint16_t reload;

    /* Enable clocks */
    Clk_SetPeripheralGate(ClkPeripheralUart0, TRUE);
    Clk_SetPeripheralGate(ClkPeripheralGpio, TRUE);
    Clk_SetPeripheralGate(ClkPeripheralBt, TRUE);

    /* Pin mux: P14=TX (SEL=6), P15=RX (SEL=6) */
    Gpio_SetFunc_UART0TX_P14();
    Gpio_SetFunc_UART0RX_P15();

    /* UART0 mode: Mode1 (8-N-1) */
    M0P_UART0->SCON = 0;
    M0P_UART0->SCON_f.SM01  = 1;   /* Mode1 */
    M0P_UART0->SCON_f.DBAUD = 0;   /* Normal baud */
    M0P_UART0->SCON_f.REN   = 1;   /* Enable receiver */

    /* BT0: auto-reload timer for baud rate clock */
    reload = 65536 - (uint16_t)((float)SystemCoreClock / (baudrate * 32) + 0.5f);

    M0P_BT0->CR = 0;
    M0P_BT0->CR_f.CT     = 0;   /* Timer mode */
    M0P_BT0->CR_f.MD     = 1;   /* Mode2: 16-bit auto-reload */
    M0P_BT0->CR_f.TOG_EN = 1;   /* Toggle output → UART0 clock */
    M0P_BT0->CR_f.PRS    = 0;   /* Div1 */
    M0P_BT0->ARR = reload;
    M0P_BT0->CNT = reload;
    M0P_BT0->CR_f.TR = 1;       /* Run */
}

void uart_send(const uint8_t *buf, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        M0P_UART0->ICR_f.TICLR = 0;
        M0P_UART0->SBUF = buf[i];
        while (!M0P_UART0->ISR_f.TI);
        M0P_UART0->ICR_f.TICLR = 0;
    }
}

uint16_t uart_recv(uint8_t *buf, uint16_t max_len) {
    uint16_t count = 0;
    while (count < max_len && M0P_UART0->ISR_f.RI) {
        buf[count++] = (uint8_t)M0P_UART0->SBUF;
        M0P_UART0->ICR_f.RICLR = 0;
    }
    return count;
}

void uart_send_str(const char *str) {
    while (*str) {
        M0P_UART0->ICR_f.TICLR = 0;
        M0P_UART0->SBUF = (uint8_t)*str++;
        while (!M0P_UART0->ISR_f.TI);
        M0P_UART0->ICR_f.TICLR = 0;
    }
}

uint8_t uart_send_async(uint8_t data) {
    if (M0P_UART0->ISR_f.TI) {
        M0P_UART0->ICR_f.TICLR = 0;
        M0P_UART0->SBUF = data;
        return 1;
    }
    return 0;
}
