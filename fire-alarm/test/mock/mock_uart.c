#include "uart_drv.h"
#include <string.h>
static char s_tx_buf[256];
static uint16_t s_tx_len;

void mock_uart_reset(void) { s_tx_buf[0]='\0'; s_tx_len=0; }
void uart_init(uint32_t baudrate) { (void)baudrate; }
void uart_send(const uint8_t *buf, uint16_t len) {
    uint16_t copy = (len < sizeof(s_tx_buf)-1) ? len : (uint16_t)(sizeof(s_tx_buf)-1);
    memcpy(s_tx_buf, buf, copy);
    s_tx_buf[copy] = '\0';
    s_tx_len = copy;
}
uint16_t uart_recv(uint8_t *buf, uint16_t max_len) { (void)buf; (void)max_len; return 0; }
void uart_send_str(const char *str) { uart_send((const uint8_t*)str, (uint16_t)strlen(str)); }
const char *mock_uart_last_sent(void) { return s_tx_buf; }
uint16_t mock_uart_sent_count(void) { return s_tx_len; }
