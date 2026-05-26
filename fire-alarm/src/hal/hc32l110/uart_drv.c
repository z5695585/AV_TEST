/* uart_drv.c — HC32L110 HAL: UART0 (P14/TX, P15/RX)
 *
 * Uses SDK Uart_ API in Mode1 (8-N-1, standard async).
 */
#include "uart_drv.h"
#include "ddl.h"
#include "uart.h"
#include "gpio.h"

void uart_init(uint32_t baudrate) {
    stc_uart_config_t      stcConfig;
    stc_uart_baud_config_t stcBaud;

    /* Enable UART0 clock */
    Clk_SetPeripheralGate(ClkPeripheralUart0, TRUE);
    Clk_SetPeripheralGate(ClkPeripheralGpio, TRUE);

    /* Configure pins to UART0 function */
    Gpio_SetFunc_UART0TX_P14();
    Gpio_SetFunc_UART0RX_P15();

    DDL_ZERO_STRUCT(stcConfig);
    stcConfig.enRunMode   = UartMode1;
    stcConfig.pstcMultiMode = NULL;
    stcConfig.pstcIrqCb   = NULL;
    stcConfig.bTouchNvic  = FALSE;

    Uart_Init(0, &stcConfig);

    DDL_ZERO_STRUCT(stcBaud);
    stcBaud.u8Mode  = 1;         /* Mode1 */
    stcBaud.bDbaud  = FALSE;     /* No double baud */
    stcBaud.u32Baud = baudrate;
    Uart_SetBaudRate(0, SystemCoreClock, &stcBaud);

    Uart_EnableFunc(0, UartTx);
    Uart_EnableFunc(0, UartRx);
}

void uart_send(const uint8_t *buf, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        while (Uart_GetStatus(0, UartTxEmpty) == FALSE);
        Uart_SendData(0, buf[i]);
    }
}

uint16_t uart_recv(uint8_t *buf, uint16_t max_len) {
    uint16_t count = 0;
    while (count < max_len && Uart_GetStatus(0, UartRxFull) != FALSE) {
        buf[count++] = Uart_ReceiveData(0);
    }
    return count;
}

void uart_send_str(const char *str) {
    while (*str) {
        while (Uart_GetStatus(0, UartTxEmpty) == FALSE);
        Uart_SendData(0, (uint8_t)*str++);
    }
}
