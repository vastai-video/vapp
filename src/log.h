
#ifndef _VAST_VAPP_LOG_H_
#define _VAST_VAPP_LOG_H_
#include <stdio.h>
enum {
    VAPP_LOG_ERROR = 0,
    VAPP_LOG_WARN,
    VAPP_LOG_INFO,
    VAPP_LOG_DEBUG,
    VAPP_LOG_TRACE,
    VAPP_LOG_COUNT
};

extern int vapp_log_level;
#define VAPP_LOG(l, ...) do {if (l <= vapp_log_level) printf(__VA_ARGS__); } while (0)
#endif
