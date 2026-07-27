#include "motor_drv8701.h"

#include "ti_msp_dl_config.h"

#include <stdbool.h>

/*
 * Board mapping:
 *   Left motor  PWM -> PB2
 *   Left motor  IN1 -> PA13
 *   Left motor  IN2 -> PA14
 *   Right motor PWM -> PB3
 *   Right motor IN1 -> PA17
 *   Right motor IN2 -> PA16
 */
#define MOTOR_SWAP_LEFT_RIGHT         (0)
#define MOTOR_PWM_PERIOD_COUNTS       (500U)
#define MOTOR_LEFT_FORWARD_IN1_HIGH   (true)
#define MOTOR_RIGHT_FORWARD_IN1_HIGH  (false)

static int16_t gLeftCommand = 0;
static int16_t gRightCommand = 0;

static int16_t motor_clamp(int32_t value)
{
    if (value > MOTOR_COMMAND_MAX) {
        return MOTOR_COMMAND_MAX;
    }
    if (value < -MOTOR_COMMAND_MAX) {
        return -MOTOR_COMMAND_MAX;
    }
    return (int16_t)value;
}

static bool motor_is_reverse(int16_t value)
{
    return value < 0;
}

static uint16_t motor_magnitude(int16_t value)
{
    int32_t magnitude = value;

    if (magnitude < 0) {
        magnitude = -magnitude;
    }

    return (uint16_t)magnitude;
}

static uint32_t motor_compare_value(int16_t command)
{
    uint32_t magnitude = motor_magnitude(command);

    return (magnitude * MOTOR_PWM_PERIOD_COUNTS) / MOTOR_COMMAND_MAX;
}

static bool motor_direction_changes(int16_t previous, int16_t next)
{
    return (previous != 0) && (next != 0) &&
        (motor_is_reverse(previous) != motor_is_reverse(next));
}

static void motor_set_pin(uint32_t pin, bool high)
{
    if (high) {
        DL_GPIO_setPins(GPIOA, pin);
    } else {
        DL_GPIO_clearPins(GPIOA, pin);
    }
}

static void motor_set_pair(
    uint32_t in1Pin, uint32_t in2Pin, bool in1High, bool in2High)
{
    motor_set_pin(in1Pin, in1High);
    motor_set_pin(in2Pin, in2High);
}

static void motor_set_side_direction(
    uint32_t in1Pin, uint32_t in2Pin, int16_t command, bool forwardIn1High)
{
    if (command == 0) {
        motor_set_pair(in1Pin, in2Pin, false, false);
        return;
    }

    if (motor_is_reverse(command)) {
        motor_set_pair(in1Pin, in2Pin, !forwardIn1High, forwardIn1High);
    } else {
        motor_set_pair(in1Pin, in2Pin, forwardIn1High, !forwardIn1High);
    }
}

static void motor_get_physical_mapping(
    uint32_t *leftPwmIdx,
    uint32_t *rightPwmIdx,
    uint32_t *leftIn1Pin,
    uint32_t *leftIn2Pin,
    uint32_t *rightIn1Pin,
    uint32_t *rightIn2Pin)
{
    if ((leftPwmIdx == NULL) || (rightPwmIdx == NULL) ||
        (leftIn1Pin == NULL) || (leftIn2Pin == NULL) ||
        (rightIn1Pin == NULL) || (rightIn2Pin == NULL)) {
        return;
    }

#if MOTOR_SWAP_LEFT_RIGHT
    *leftPwmIdx = GPIO_MOTOR_PWM_C1_IDX;
    *rightPwmIdx = GPIO_MOTOR_PWM_C0_IDX;
    *leftIn1Pin = MOTOR_DIR_RIGHT_IN1_PIN;
    *leftIn2Pin = MOTOR_DIR_RIGHT_IN2_PIN;
    *rightIn1Pin = MOTOR_DIR_LEFT_IN1_PIN;
    *rightIn2Pin = MOTOR_DIR_LEFT_IN2_PIN;
#else
    *leftPwmIdx = GPIO_MOTOR_PWM_C0_IDX;
    *rightPwmIdx = GPIO_MOTOR_PWM_C1_IDX;
    *leftIn1Pin = MOTOR_DIR_LEFT_IN1_PIN;
    *leftIn2Pin = MOTOR_DIR_LEFT_IN2_PIN;
    *rightIn1Pin = MOTOR_DIR_RIGHT_IN1_PIN;
    *rightIn2Pin = MOTOR_DIR_RIGHT_IN2_PIN;
#endif
}

void Motor_init(void)
{
    uint32_t leftPwmIdx;
    uint32_t rightPwmIdx;
    uint32_t leftIn1Pin;
    uint32_t leftIn2Pin;
    uint32_t rightIn1Pin;
    uint32_t rightIn2Pin;

    motor_get_physical_mapping(
        &leftPwmIdx, &rightPwmIdx,
        &leftIn1Pin, &leftIn2Pin,
        &rightIn1Pin, &rightIn2Pin);

    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, 0U, leftPwmIdx);
    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, 0U, rightPwmIdx);

    motor_set_pair(leftIn1Pin, leftIn2Pin, false, false);
    motor_set_pair(rightIn1Pin, rightIn2Pin, false, false);

    gLeftCommand = 0;
    gRightCommand = 0;
    DL_TimerG_startCounter(MOTOR_PWM_INST);
}

void Motor_stop(void)
{
    uint32_t leftPwmIdx;
    uint32_t rightPwmIdx;
    uint32_t leftIn1Pin;
    uint32_t leftIn2Pin;
    uint32_t rightIn1Pin;
    uint32_t rightIn2Pin;

    motor_get_physical_mapping(
        &leftPwmIdx, &rightPwmIdx,
        &leftIn1Pin, &leftIn2Pin,
        &rightIn1Pin, &rightIn2Pin);

    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, 0U, leftPwmIdx);
    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, 0U, rightPwmIdx);
    motor_set_pair(leftIn1Pin, leftIn2Pin, false, false);
    motor_set_pair(rightIn1Pin, rightIn2Pin, false, false);
    gLeftCommand = 0;
    gRightCommand = 0;
}

void Motor_drive(int leftCommand, int rightCommand)
{
    uint32_t leftPwmIdx;
    uint32_t rightPwmIdx;
    uint32_t leftIn1Pin;
    uint32_t leftIn2Pin;
    uint32_t rightIn1Pin;
    uint32_t rightIn2Pin;
    int16_t left = motor_clamp(leftCommand);
    int16_t right = motor_clamp(rightCommand);
    bool leftDirectionChanges = motor_direction_changes(gLeftCommand, left);
    bool rightDirectionChanges =
        motor_direction_changes(gRightCommand, right);

    motor_get_physical_mapping(
        &leftPwmIdx, &rightPwmIdx,
        &leftIn1Pin, &leftIn2Pin,
        &rightIn1Pin, &rightIn2Pin);

    if (leftDirectionChanges) {
        DL_TimerG_setCaptureCompareValue(
            MOTOR_PWM_INST, 0U, leftPwmIdx);
    }
    if (rightDirectionChanges) {
        DL_TimerG_setCaptureCompareValue(
            MOTOR_PWM_INST, 0U, rightPwmIdx);
    }
    if (leftDirectionChanges || rightDirectionChanges) {
        delay_cycles((CPUCLK_FREQ / 1000U) + 1U);
    }

    motor_set_side_direction(
        leftIn1Pin,
        leftIn2Pin,
        left,
        MOTOR_LEFT_FORWARD_IN1_HIGH);
    motor_set_side_direction(
        rightIn1Pin,
        rightIn2Pin,
        right,
        MOTOR_RIGHT_FORWARD_IN1_HIGH);

    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, motor_compare_value(left), leftPwmIdx);
    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, motor_compare_value(right), rightPwmIdx);

    gLeftCommand = left;
    gRightCommand = right;
}

int16_t Motor_getLeftCommand(void)
{
    return gLeftCommand;
}

int16_t Motor_getRightCommand(void)
{
    return gRightCommand;
}
