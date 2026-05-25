#include "test_framework.h"
#include "mock_pwm.h"
#include "mock_timer.h"
#include "alarm_light.h"

TEST(alarm_light_basic) {
    mock_pwm_reset();
    mock_timer_set_tick(0);
    alarm_light_init();

    uint8_t ok = alarm_light_load(0);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(alarm_light_is_active());
    ASSERT_TRUE(mock_pwm_is_running(1));

    ASSERT_FALSE(mock_pwm_is_running(0));

    alarm_light_stop();
    ASSERT_FALSE(mock_pwm_is_running(1));
}
