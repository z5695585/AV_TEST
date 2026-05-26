/* uart_drv.c — HC32L110 HAL: UART1 (P23/TX pin13, P24/RX pin14)
 *
 * Uses SDK Uart_ API in Mode1 (8-N-1, standard async).
 */
#include "uart_drv.h"
#include "ddl.h"
#include "uart.h"
#include "gpio.h"

#define UART_IDX  1  /* UART1 */

void uart_init(uint32_t baudrate) {
    stc_uart_config_t      stcConfig;
    stc_uart_baud_config_t stcBaud;

    Clk_SetPeripheralGate(ClkPeripheralUart1, TRUE);
    Clk_SetPeripheralGate(ClkPeripheralGpio, TRUE);

    Gpio_SetFunc_UART1TX_P23();
    Gpio_SetFunc_UART1RX_P24();

    DDL_ZERO_STRUCT(stcConfig);
    stcConfig.enRunMode     = UartMode1;
    stcConfig.pstcMultiMode = NULL;
    stcConfig.pstcIrqCb     = NULL;
    stcConfig.bTouchNvic    = FALSE;

    Uart_Init(UART_IDX, &stcConfig);

    DDL_ZERO_STRUCT(stcBaud);
    stcBaud.u8Mode  = 1;
    stcBaud.bDbaud  = FALSE;
    stcBaud.u32Baud = baudrate;
    Uart_SetBaudRate(UART_IDX, SystemCoreClock, &stcBaud);

    Uart_EnableFunc(UART_IDX, UartTx);
    Uart_EnableFunc(UART_IDX, UartRx);
}

void uart_send(const uint8_t *buf, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        while (Uart_GetStatus(UART_IDX, UartTxEmpty) == FALSE);
        Uart_SendData(UART_IDX, buf[i]);
    }
}

uint16_t uart_recv(uint8_t *buf, uint16_t max_len) {
    uint16_t count = 0;
    while (count < max_len && Uart_GetStatus(UART_IDX, UartRxFull) != FALSE) {
        buf[count++] = Uart_ReceiveData(UART_IDX);
    }
    return count;
}

void uart_send_str(const char *str) {
    while (*str) {
        while (Uart_GetStatus(UART_IDX, UartTxEmpty) == FALSE);
        Uart_SendData(UART_IDX, (uint8_t)*str++);
    }
}
