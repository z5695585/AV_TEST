#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H
#include <stdio.h>
#include <string.h>

extern int s_passed;
extern int s_failed;

#define TEST(name) void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  RUN  %s\n", #name); \
    test_##name(); \
} while(0)

#define ASSERT_EQ(expected, actual, fmt) do { \
    if ((expected) != (actual)) { \
        printf("  FAIL %s:%d: expected " fmt " got " fmt "\n", \
               __FILE__, __LINE__, (expected), (actual)); \
        s_failed++; return; \
    } \
    s_passed++; \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("  FAIL %s:%d: expected \"%s\" got \"%s\"\n", \
               __FILE__, __LINE__, (expected), (actual)); \
        s_failed++; return; \
    } \
    s_passed++; \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        printf("  FAIL %s:%d: expected true\n", __FILE__, __LINE__); \
        s_failed++; return; \
    } \
    s_passed++; \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#endif
