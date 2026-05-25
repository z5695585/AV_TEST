#include "test_framework.h"
#include "mock_pwm.h"
#include "mock_timer.h"
#include "timer_drv.h"
#include "alarm_sound.h"

TEST(alarm_sound_basic) {
    mock_pwm_reset();
    mock_timer_set_tick(0);
    alarm_sound_init();

    uint8_t ok = alarm_sound_load(0);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(alarm_sound_is_active());
    ASSERT_TRUE(mock_pwm_is_running(0));
    ASSERT_EQ((uint32_t)3000, mock_pwm_get_freq(0), "%lu");
    ASSERT_EQ((uint8_t)50, mock_pwm_get_duty(0), "%u");

    alarm_sound_stop();
    ASSERT_FALSE(alarm_sound_is_active());
    ASSERT_FALSE(mock_pwm_is_running(0));

    ok = alarm_sound_load(200);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(alarm_sound_is_active());
}

TEST(alarm_sound_advance) {
    mock_pwm_reset();
    mock_timer_set_tick(0);
    alarm_sound_init();
    alarm_sound_load(1);  /* P1: T3 large, 500ms on/off */

    ASSERT_TRUE(mock_pwm_is_running(0));

    mock_timer_advance(500);
    alarm_sound_tick(timer_get_tick());
    ASSERT_FALSE(mock_pwm_is_running(0));

    mock_timer_advance(500);
    alarm_sound_tick(timer_get_tick());
    ASSERT_TRUE(mock_pwm_is_running(0));
}
