#ifndef MOTOR_TB6612_INTERFACE_H
#define MOTOR_TB6612_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    MOTOR_TB6612_LEFT = 0,
    MOTOR_TB6612_RIGHT = 1
} MotorTb6612Channel;

typedef enum {
    MOTOR_TB6612_STOP = 0,
    MOTOR_TB6612_FORWARD,
    MOTOR_TB6612_REVERSE
} MotorTb6612Direction;

/*
 * PWM uses permille: 0 = 0%, 1000 = 100%.
 * set_motor_output must update direction safely and must never exceed the
 * requested PWM. The board adapter owns concrete SysConfig pin names.
 */
typedef struct {
    uint32_t (*get_time_ms)(void);
    bool (*is_estop_asserted)(void);
    void (*set_standby)(bool enabled);
    void (*set_motor_output)(MotorTb6612Channel channel,
                             MotorTb6612Direction direction,
                             uint16_t pwm_permille);
} MotorTb6612Interface;

#endif
