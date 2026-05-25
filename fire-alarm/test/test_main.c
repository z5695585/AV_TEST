#include "test_framework.h"

int s_passed = 0;
int s_failed = 0;

/* Forward declarations */
TEST(pattern_table);
TEST(alarm_sound_basic);
TEST(alarm_sound_advance);
TEST(alarm_light_basic);
TEST(protocol_buzz_on_off);
TEST(protocol_led_on_off);
TEST(protocol_dip_status);
TEST(protocol_unknown_cmd);
TEST(protocol_range_check);

int main(void) {
    printf("=== Fire Alarm Unit Tests ===\n\n");

    printf("Pattern Table:\n");
    RUN_TEST(pattern_table);

    printf("\nAlarm Sound:\n");
    RUN_TEST(alarm_sound_basic);
    RUN_TEST(alarm_sound_advance);

    printf("\nAlarm Light:\n");
    RUN_TEST(alarm_light_basic);

    printf("\nTest Protocol:\n");
    RUN_TEST(protocol_buzz_on_off);
    RUN_TEST(protocol_led_on_off);
    RUN_TEST(protocol_dip_status);
    RUN_TEST(protocol_unknown_cmd);
    RUN_TEST(protocol_range_check);

    printf("\n=== Results: %d passed, %d failed ===\n", s_passed, s_failed);
    return s_failed ? 1 : 0;
}
