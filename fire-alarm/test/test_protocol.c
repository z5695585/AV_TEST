#include "test_framework.h"
#include "mock_uart.h"
#include "test_protocol.h"

static uint8_t g_buzz_idx = 0xFF, g_led_idx = 0xFF;
static uint8_t g_s_dip = 0, g_l_dip = 0;

void proto_buzz_load(uint8_t idx) { g_buzz_idx = idx; }
void proto_buzz_stop(void)         { g_buzz_idx = 0xFF; }
void proto_led_load(uint8_t idx)   { g_led_idx = idx; }
void proto_led_stop(void)          { g_led_idx = 0xFF; }
void proto_reset_to_dip(void)      { g_buzz_idx = g_s_dip; g_led_idx = g_l_dip; }

static void feed_str(proto_ctx_t *ctx, const char *str) {
    proto_feed(ctx, (const uint8_t*)str, (uint16_t)strlen(str));
}

TEST(protocol_buzz_on_off) {
    mock_uart_reset();
    proto_ctx_t ctx;
    proto_init(&ctx, NULL, 0);
    proto_update_state(&ctx, 0, 0, 0xFF, 0xFF);

    feed_str(&ctx, "BUZZ ON 0\n");
    ASSERT_EQ((uint8_t)0, g_buzz_idx, "%u");
    ASSERT_STR_EQ("OK BUZZ=0\r\n", mock_uart_last_sent());

    feed_str(&ctx, "BUZZ OFF\n");
    ASSERT_EQ((uint8_t)0xFF, g_buzz_idx, "%u");
    ASSERT_STR_EQ("OK BUZZ=OFF\r\n", mock_uart_last_sent());
}

TEST(protocol_led_on_off) {
    mock_uart_reset();
    proto_ctx_t ctx;
    proto_init(&ctx, NULL, 0);

    feed_str(&ctx, "LED ON 1\n");
    ASSERT_EQ((uint8_t)1, g_led_idx, "%u");
    ASSERT_STR_EQ("OK LED=1\r\n", mock_uart_last_sent());

    feed_str(&ctx, "LED OFF\n");
    ASSERT_EQ((uint8_t)0xFF, g_led_idx, "%u");
}

TEST(protocol_dip_status) {
    mock_uart_reset();
    proto_ctx_t ctx;
    proto_init(&ctx, NULL, 0);
    g_s_dip = 2; g_l_dip = 1;
    proto_update_state(&ctx, 2, 1, 0xFF, 0xFF);

    feed_str(&ctx, "DIP\n");
    ASSERT_STR_EQ("OK SD=2 LD=1\r\n", mock_uart_last_sent());

    feed_str(&ctx, "STATUS\n");
    ASSERT_STR_EQ("OK B=255 L=255 SD=2 LD=1\r\n", mock_uart_last_sent());
}

TEST(protocol_unknown_cmd) {
    mock_uart_reset();
    proto_ctx_t ctx;
    proto_init(&ctx, NULL, 0);

    feed_str(&ctx, "FOOBAR\n");
    ASSERT_STR_EQ("ERR CMD\r\n", mock_uart_last_sent());
}

TEST(protocol_range_check) {
    mock_uart_reset();
    proto_ctx_t ctx;
    proto_init(&ctx, NULL, 0);

    feed_str(&ctx, "BUZZ ON 99\n");
    ASSERT_STR_EQ("ERR RANGE\r\n", mock_uart_last_sent());
}
