#ifndef MOCK_UART_H
#define MOCK_UART_H
#include <stdint.h>
void mock_uart_reset(void);
const char *mock_uart_last_sent(void);
uint16_t mock_uart_sent_count(void);
#endif
