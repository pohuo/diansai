#include "motor_tb6612.h"

#include "ti_msp_dl_config.h"

#include <stdbool.h>

/*
 * TB6612 mapping used by the current board:
 *   Left  motor PWM -> PB2  (TIMG6_CCP0)
 *   Left  motor IN1  -> PA13
 *   Left  motor IN2  -> PA14
 *   Right motor PWM -> PB3  (TIMG6_CCP1)
 *   Right motor IN1  -> PA17
 *   Right motor IN2  -> PA16
 */
#define MOTOR_PWM_PERIOD_COUNTS       (500U)
#define MOTOR_LEFT_FORWARD_IN1_HIGH   (true)
#define MOTOR_RIGHT_FORWARD_IN1_HIGH  (true)

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
        DL_GPIO_setPins(MOTOR_DIR_PORT, pin);
    } else {
        DL_GPIO_clearPins(MOTOR_DIR_PORT, pin);
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

void Motor_init(void)
{
    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, 0U, GPIO_MOTOR_PWM_C0_IDX);
    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, 0U, GPIO_MOTOR_PWM_C1_IDX);

    motor_set_pair(
        MOTOR_DIR_LEFT_IN1_PIN, MOTOR_DIR_LEFT_IN2_PIN, false, false);
    motor_set_pair(
        MOTOR_DIR_RIGHT_IN1_PIN, MOTOR_DIR_RIGHT_IN2_PIN, false, false);

    gLeftCommand = 0;
    gRightCommand = 0;
    DL_TimerG_startCounter(MOTOR_PWM_INST);
}

void Motor_stop(void)
{
    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, 0U, GPIO_MOTOR_PWM_C0_IDX);
    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, 0U, GPIO_MOTOR_PWM_C1_IDX);
    motor_set_pair(
        MOTOR_DIR_LEFT_IN1_PIN, MOTOR_DIR_LEFT_IN2_PIN, false, false);
    motor_set_pair(
        MOTOR_DIR_RIGHT_IN1_PIN, MOTOR_DIR_RIGHT_IN2_PIN, false, false);
    gLeftCommand = 0;
    gRightCommand = 0;
}

void Motor_drive(int leftCommand, int rightCommand)
{
    int16_t left = motor_clamp(leftCommand);
    int16_t right = motor_clamp(rightCommand);
    bool leftDirectionChanges = motor_direction_changes(gLeftCommand, left);
    bool rightDirectionChanges = motor_direction_changes(gRightCommand, right);

    if (leftDirectionChanges) {
        DL_TimerG_setCaptureCompareValue(
            MOTOR_PWM_INST, 0U, GPIO_MOTOR_PWM_C0_IDX);
    }
    if (rightDirectionChanges) {
        DL_TimerG_setCaptureCompareValue(
            MOTOR_PWM_INST, 0U, GPIO_MOTOR_PWM_C1_IDX);
    }
    if (leftDirectionChanges || rightDirectionChanges) {
        delay_cycles((CPUCLK_FREQ / 1000U) + 1U);
    }

    motor_set_side_direction(
        MOTOR_DIR_LEFT_IN1_PIN,
        MOTOR_DIR_LEFT_IN2_PIN,
        left,
        MOTOR_LEFT_FORWARD_IN1_HIGH);
    motor_set_side_direction(
        MOTOR_DIR_RIGHT_IN1_PIN,
        MOTOR_DIR_RIGHT_IN2_PIN,
        right,
        MOTOR_RIGHT_FORWARD_IN1_HIGH);

    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, motor_compare_value(left), GPIO_MOTOR_PWM_C0_IDX);
    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, motor_compare_value(right), GPIO_MOTOR_PWM_C1_IDX);

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
