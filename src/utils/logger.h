#ifndef LOGGER_H
#define LOGGER_H

#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// ------------------- Simple USB CDC Logger -------------------
// Log levels (hierarchical)
#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DEBUG 4

// Set the current log level for the project
// Change to LOG_LEVEL_INFO or LOG_LEVEL_WARN for release builds
#ifndef CURRENT_LOG_LEVEL
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#endif

// ANSI color codes for terminal output
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Helper macro to extract just the filename from __FILE__
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : \
                     (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__))

// Log macros with timestamp, file, line, and module tags
#define LOGE(tag, ...) do { \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_ERROR) { \
        printf(ANSI_COLOR_RED "[E][%lu][%s:%d][%s] ", \
               to_ms_since_boot(get_absolute_time()), __FILENAME__, __LINE__, tag); \
        printf(__VA_ARGS__); \
        printf(ANSI_COLOR_RESET "\r\n"); \
    } \
} while(0)

#define LOGW(tag, ...) do { \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_WARN) { \
        printf(ANSI_COLOR_YELLOW "[W][%lu][%s:%d][%s] ", \
               to_ms_since_boot(get_absolute_time()), __FILENAME__, __LINE__, tag); \
        printf(__VA_ARGS__); \
        printf(ANSI_COLOR_RESET "\r\n"); \
    } \
} while(0)

#define LOGI(tag, ...) do { \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_INFO) { \
        printf(ANSI_COLOR_GREEN "[I][%lu][%s:%d][%s] ", \
               to_ms_since_boot(get_absolute_time()), __FILENAME__, __LINE__, tag); \
        printf(__VA_ARGS__); \
        printf(ANSI_COLOR_RESET "\r\n"); \
    } \
} while(0)

#define LOGD(tag, ...) do { \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG) { \
        printf("[D][%lu][%s:%d][%s] ", \
               to_ms_since_boot(get_absolute_time()), __FILENAME__, __LINE__, tag); \
        printf(__VA_ARGS__); \
        printf("\r\n"); \
    } \
} while(0)

#endif // LOGGER_H
