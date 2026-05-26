#include "test_protocol.h"
#include "pattern_table.h"
#include "uart_drv.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* --- External callbacks, implemented in main.c --- */
extern void proto_buzz_load(uint8_t idx);
extern void proto_buzz_stop(void);
extern void proto_led_load(uint8_t idx);
extern void proto_led_stop(void);
extern void proto_reset_to_dip(void);

static void proto_reply(const char *msg);

/* Tokenize: split input line into cmd + argv. Returns argc (including cmd). */
static uint8_t parse_line(char *line, char *argv[], uint8_t max_args) {
    uint8_t argc = 0;
    char *token = line;
    char *next;
    while (argc < max_args + 1) {
        while (*token == ' ') token++;
        if (*token == '\0' || *token == '\r') break;
        next = token;
        while (*next != ' ' && *next != '\0' && *next != '\r') next++;
        if (argc == 0) {
            argv[0] = token;
        } else {
            argv[argc] = token;
        }
        argc++;
        if (*next == '\0' || *next == '\r') break;
        *next = '\0';
        token = next + 1;
    }
    return argc;
}

/* --- Command handlers ---
 *
 * The tokenizer splits "BUZZ ON 2" into argv[0]="ON", argv[1]="2"
 * and "BUZZ OFF" into argv[0]="OFF".  Both come through the same
 * "BUZZ" command table entry.  We check argv[0] to distinguish.
 */

static void h_buzz(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    if (argc >= 1 && strcmp(argv[0], "OFF") == 0) {
        proto_buzz_stop();
        ctx->buzz_idx = 0xFF;
        proto_reply("OK BUZZ=OFF\r\n");
        return;
    }
    /* BUZZ ON <n> or BUZZ <n> */
    if (argc < 2) { proto_reply("ERR CMD\r\n"); return; }
    uint8_t idx = (uint8_t)atoi(argv[argc - 1]);
    if (idx >= MAX_PATTERNS) { proto_reply("ERR RANGE\r\n"); return; }
    proto_buzz_load(idx);
    ctx->buzz_idx = idx;
    char rsp[32];
    snprintf(rsp, sizeof(rsp), "OK BUZZ=%d\r\n", idx);
    proto_reply(rsp);
}

static void h_led(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    if (argc >= 1 && strcmp(argv[0], "OFF") == 0) {
        proto_led_stop();
        ctx->led_idx = 0xFF;
        proto_reply("OK LED=OFF\r\n");
        return;
    }
    if (argc < 2) { proto_reply("ERR CMD\r\n"); return; }
    uint8_t idx = (uint8_t)atoi(argv[argc - 1]);
    if (idx >= MAX_PATTERNS) { proto_reply("ERR RANGE\r\n"); return; }
    proto_led_load(idx);
    ctx->led_idx = idx;
    char rsp[32];
    snprintf(rsp, sizeof(rsp), "OK LED=%d\r\n", idx);
    proto_reply(rsp);
}

static void h_dip(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    (void)argc; (void)argv;
    char rsp[32];
    snprintf(rsp, sizeof(rsp), "OK SD=%d LD=%d\r\n", ctx->sound_dip, ctx->light_dip);
    proto_reply(rsp);
}

static void h_status(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    (void)argc; (void)argv;
    char rsp[48];
    snprintf(rsp, sizeof(rsp), "OK B=%d L=%d SD=%d LD=%d\r\n",
             ctx->buzz_idx == 0xFF ? 255 : ctx->buzz_idx,
             ctx->led_idx == 0xFF ? 255 : ctx->led_idx,
             ctx->sound_dip, ctx->light_dip);
    proto_reply(rsp);
}

static void h_reset(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    (void)ctx; (void)argc; (void)argv;
    proto_reset_to_dip();
    proto_reply("OK RESET\r\n");
}

static void h_test(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    (void)ctx; (void)argc; (void)argv;
    proto_reply("OK ALARM_TEST\r\n");
    proto_buzz_load(0);
    proto_led_load(0);
}

/* --- Command table --- */

static void h_ping(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    (void)ctx; (void)argc; (void)argv;
    proto_reply("PONG\r\n");
}

static void h_help(proto_ctx_t *ctx, uint8_t argc, char *argv[]) {
    (void)ctx; (void)argc; (void)argv;
    proto_reply("OK HELP: BUZZ ON|OFF <n>, LED ON|OFF <n>, DIP, STATUS, RESET, TEST, PING, HELP\r\n");
}

static const cmd_entry_t default_commands[] = {
    {"BUZZ",    1, h_buzz},
    {"LED",     1, h_led},
    {"DIP",     0, h_dip},
    {"STATUS",  0, h_status},
    {"RESET",   0, h_reset},
    {"TEST",    0, h_test},
    {"PING",    0, h_ping},
    {"HELP",    0, h_help},
};

/* --- API --- */

void proto_init(proto_ctx_t *ctx, const cmd_entry_t *table, uint8_t count) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->buf[0] = '\0';
    ctx->len = 0;
    ctx->cmd_table = table ? table : default_commands;
    ctx->cmd_count = table ? count : (sizeof(default_commands) / sizeof(default_commands[0]));
    ctx->buzz_idx = 0xFF;
    ctx->led_idx = 0xFF;
}

void proto_update_state(proto_ctx_t *ctx, uint8_t s_dip, uint8_t l_dip, uint8_t buzz, uint8_t led) {
    ctx->sound_dip = s_dip;
    ctx->light_dip = l_dip;
    ctx->buzz_idx = buzz;
    ctx->led_idx = led;
}

void proto_feed(proto_ctx_t *ctx, const uint8_t *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        char c = (char)data[i];
        if (c == '\r') continue;

        if (c == '\n') {
            ctx->buf[ctx->len] = '\0';
            ctx->len = 0;

            if (ctx->buf[0] == '\0') continue;

            char *argv[PROTO_MAX_ARGS + 1] = {0};
            uint8_t argc = parse_line(ctx->buf, argv, PROTO_MAX_ARGS);

            if (argc == 0) {
                proto_reply("ERR CMD\r\n");
                continue;
            }

            uint8_t found = 0;
            for (uint8_t j = 0; j < ctx->cmd_count; j++) {
                if (strcmp(argv[0], ctx->cmd_table[j].cmd) == 0) {
                    if (argc - 1 < ctx->cmd_table[j].min_args) {
                        proto_reply("ERR CMD\r\n");
                    } else {
                        ctx->cmd_table[j].handler(ctx, argc - 1, argv + 1);
                    }
                    found = 1;
                    break;
                }
            }
            if (!found) {
                proto_reply("ERR CMD\r\n");
            }
        } else {
            if (ctx->len < PROTO_BUF_SIZE - 1) {
                ctx->buf[ctx->len++] = c;
            }
        }
    }
}

static void proto_reply(const char *msg) {
    uart_send_str(msg);
}
