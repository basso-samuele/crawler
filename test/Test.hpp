#pragma once

#include <iostream>
#include <format>
#include <cstring>

static int testCount = 0;
static int testFailed = 0;
static int testResult = 0;

#define CRAWLER_TEST_RESULT testResult

#define CRAWLER_PRINT_TEST_SUMMARY                                              \
    do {                                                                        \
        std::cout <<                                                            \
            std::format(                                                        \
                "Test execution summary.\n\tCount: {}\n\tFailed: {} - {:.2f}%", \
                testCount, testFailed, (testFailed * 100.0 / testCount)         \
            )                                                                   \
        << std::endl;                                                           \
    } while(0)

#define _CRAWLER_ASSERT_MEMEQ(expected, actual, count, ...)       \
    if (memcmp(expected, actual, count)) {                        \
        std::cout <<                                              \
            std::format(                                          \
                "Assertion {}:{}:{} failed" __VA_OPT__("\n\t{}"), \
                __FILE__, __func__, __LINE__                      \
                __VA_OPT__(,std::format(__VA_ARGS__))             \
            )                                                     \
        << std::endl;                                             \
        testResult = 1;                                           \
        testFailed++;                                             \
    }

#define _CRAWLER_ASSERT_EQ(expected, actual, ...)                                                  \
    if (expected != actual) {                                                                      \
        std::cout <<                                                                               \
            std::format(                                                                           \
                "Assertion {}:{}:{} failed" __VA_OPT__("\n\t{}") "\n\tExpected: {}\n\tActual: {}", \
                __FILE__, __func__, __LINE__,                                                      \
                __VA_OPT__(std::format(__VA_ARGS__),)                                              \
                expected, actual                                                                   \
            )                                                                                      \
        << std::endl;                                                                              \
        testResult = 1;                                                                            \
        testFailed++;                                                                              \
    }

#define CRAWLER_ASSERT_MEMEQ(expected, actual, count, ...)          \
    do {                                                            \
        testCount++;                                                \
        _CRAWLER_ASSERT_MEMEQ(expected, actual, count, __VA_ARGS__) \
    } while(0)

#define CRAWLER_ASSERT_EQ(expected, actual, ...)          \
    do {                                                  \
        testCount++;                                      \
        _CRAWLER_ASSERT_EQ(expected, actual, __VA_ARGS__) \
    } while(0)

#define CRAWLER_ASSERT_TRUE(actual, ...) \
    CRAWLER_ASSERT_EQ(true, actual, __VA_ARGS__)

#define CRAWLER_ASSERT_FALSE(actual, ...) \
    CRAWLER_ASSERT_EQ(false, actual, __VA_ARGS__)
