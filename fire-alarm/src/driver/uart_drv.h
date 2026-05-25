#ifndef UART_DRV_H
#define UART_DRV_H
#include <stdint.h>

void     uart_init(uint32_t baudrate);
void     uart_send(const uint8_t *buf, uint16_t len);
uint16_t uart_recv(uint8_t *buf, uint16_t max_len);
void     uart_send_str(const char *str);

#endif
