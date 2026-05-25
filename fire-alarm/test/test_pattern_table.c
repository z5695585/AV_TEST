#include "test_framework.h"
#include "pattern_table.h"

TEST(pattern_table) {
    ASSERT_TRUE(sound_pattern_table[0].step_count > 0);
    ASSERT_TRUE(sound_pattern_table[0].steps != NULL);
    ASSERT_TRUE(light_pattern_table[0].step_count > 0);
    ASSERT_TRUE(light_pattern_table[0].steps != NULL);

    ASSERT_EQ((uint8_t)0, sound_pattern_table[0].loop_count, "%u");
}
