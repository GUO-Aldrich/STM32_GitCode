#ifndef LOG_COLOR_H
#define LOG_COLOR_H

#include <string.h>

#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

#define COLOR_RESET  "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"

// 带级别标签的日志宏
#define LOG_INFO(fmt, ...)  \
    printf(COLOR_GREEN  "[INFO] %s:%d " fmt COLOR_RESET, __FILENAME__, __LINE__, ##__VA_ARGS__)
    
#define LOG_WARN(fmt, ...)  \
    printf(COLOR_YELLOW "[WARN] %s:%d" fmt COLOR_RESET, __FILENAME__, __LINE__, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    printf(COLOR_RED "[ERROR] %s:%d" fmt COLOR_RESET, __FILENAME__, __LINE__, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...) \
    printf(COLOR_CYAN "[DEBUG] %s:%d" fmt COLOR_RESET, __FILENAME__, __LINE__, ##__VA_ARGS__)

#endif