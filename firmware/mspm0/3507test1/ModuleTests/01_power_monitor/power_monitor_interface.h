#ifndef POWER_MONITOR_INTERFACE_H
#define POWER_MONITOR_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t (*get_time_ms)(void);
    bool (*read_battery_adc)(uint16_t *raw_adc);
    void (*handle_low_voltage)(void);
} PowerMonitorInterface;

#endif