#ifndef BUTTON_ESTOP_INTERFACE_H
#define BUTTON_ESTOP_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Board adapter contract. Implement these callbacks with SysConfig-generated
 * GPIO names after the pin map is frozen. All input callbacks must be
 * non-blocking.
 */
typedef struct {
    uint32_t (*get_time_ms)(void);
    bool (*read_mode_pressed)(void);
    bool (*read_estop_asserted)(void);
    void (*force_actuators_safe)(void);
} ButtonEstopInterface;

#endif
