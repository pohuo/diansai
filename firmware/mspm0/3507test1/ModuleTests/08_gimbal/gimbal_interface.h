#ifndef GIMBAL_INTERFACE_H
#define GIMBAL_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    GIMBAL_AXIS_PAN = 0,
    GIMBAL_AXIS_TILT
} GimbalAxis;

typedef enum {
    GIMBAL_DIRECTION_NEGATIVE = -1,
    GIMBAL_DIRECTION_NONE = 0,
    GIMBAL_DIRECTION_POSITIVE = 1
} GimbalDirection;

typedef struct {
    uint32_t (*get_time_ms)(void);
    bool (*is_estop_asserted)(void);
    void (*set_enabled)(bool enabled);
    bool (*command_angle_mdeg)(GimbalAxis axis, int32_t target_mdeg);
    bool (*is_at_target)(GimbalAxis axis,
                         int32_t target_mdeg,
                         uint32_t tolerance_mdeg);
    bool (*is_limit_active)(GimbalAxis axis, GimbalDirection direction);
} GimbalInterface;

#endif