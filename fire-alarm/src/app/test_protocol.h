#ifndef TEST_PROTOCOL_H
#define TEST_PROTOCOL_H
#include <stdint.h>

#define PROTO_BUF_SIZE  64
#define PROTO_MAX_ARGS  2

typedef struct proto_ctx_t proto_ctx_t;

typedef void (*cmd_handler_t)(proto_ctx_t *ctx, uint8_t argc, char *argv[]);

typedef struct {
    const char    *cmd;
    uint8_t        min_args;
    cmd_handler_t  handler;
} cmd_entry_t;

struct proto_ctx_t {
    char     buf[PROTO_BUF_SIZE];
    uint8_t  len;
    const cmd_entry_t *cmd_table;
    uint8_t  cmd_count;
    uint8_t  sound_dip;
    uint8_t  light_dip;
    uint8_t  buzz_idx;
    uint8_t  led_idx;
};

void proto_init(proto_ctx_t *ctx, const cmd_entry_t *table, uint8_t count);
void proto_feed(proto_ctx_t *ctx, const uint8_t *data, uint16_t len);
void proto_update_state(proto_ctx_t *ctx, uint8_t s_dip, uint8_t l_dip, uint8_t buzz, uint8_t led);

#endif
