#include "drv8701.h"

#include "ti_msp_dl_config.h"

#define DRV8701_PWM_PERIOD_COUNTS \
    (DRV8701_PWM_INST_CLK_FREQ / DRV8701_PWM_FREQUENCY_HZ)

static int16_t gLeftCommand = 0;
static int16_t gRightCommand = 0;

static int16_t drv8701_clamp_command(int32_t command)
{
    if (command > DRV8701_COMMAND_MAX) {
        return DRV8701_COMMAND_MAX;
    }
    if (command < -DRV8701_COMMAND_MAX) {
        return -DRV8701_COMMAND_MAX;
    }
    return (int16_t) command;
}

static bool drv8701_command_is_reverse(int16_t command)
{
    return command < 0;
}

static uint16_t drv8701_command_magnitude(int16_t command)
{
    int32_t magnitude = command;

    if (magnitude < 0) {
        magnitude = -magnitude;
    }
    return (uint16_t) magnitude;
}

static uint32_t drv8701_command_to_compare(int16_t command)
{
    uint32_t magnitude = drv8701_command_magnitude(command);

    return (magnitude * DRV8701_PWM_PERIOD_COUNTS) /
        (uint32_t) DRV8701_COMMAND_MAX;
}

static bool drv8701_ph_high_for_command(
    int16_t command, bool forwardPhHigh)
{
    if (drv8701_command_is_reverse(command)) {
        return !forwardPhHigh;
    }
    return forwardPhHigh;
}

static bool drv8701_direction_changes(int16_t previous, int16_t next)
{
    return (previous != 0) && (next != 0) &&
        (drv8701_command_is_reverse(previous) !=
            drv8701_command_is_reverse(next));
}

static void drv8701_write_ph(uint32_t pin, bool high)
{
    if (high) {
        DL_GPIO_setPins(DRV8701_DIR_PORT, pin);
    } else {
        DL_GPIO_clearPins(DRV8701_DIR_PORT, pin);
    }
}

void DRV8701_init(void)
{
    /*
     * SYSCFG_DL_init() must run before this function. Keep EN at 0 while PH
     * is established, then start the shared timer.
     */
    DL_TimerG_setCaptureCompareValue(
        DRV8701_PWM_INST, 0U, GPIO_DRV8701_PWM_C0_IDX);
    DL_TimerG_setCaptureCompareValue(
        DRV8701_PWM_INST, 0U, GPIO_DRV8701_PWM_C1_IDX);

    drv8701_write_ph(
        DRV8701_DIR_LEFT_PH_PIN, DRV8701_LEFT_FORWARD_PH_HIGH);
    drv8701_write_ph(
        DRV8701_DIR_RIGHT_PH_PIN, DRV8701_RIGHT_FORWARD_PH_HIGH);

    gLeftCommand = 0;
    gRightCommand = 0;
    DL_TimerG_startCounter(DRV8701_PWM_INST);
}

void DRV8701_brake(void)
{
    DL_TimerG_setCaptureCompareValue(
        DRV8701_PWM_INST, 0U, GPIO_DRV8701_PWM_C0_IDX);
    DL_TimerG_setCaptureCompareValue(
        DRV8701_PWM_INST, 0U, GPIO_DRV8701_PWM_C1_IDX);
    gLeftCommand = 0;
    gRightCommand = 0;
}

void DRV8701_setMotors(int16_t leftCommand, int16_t rightCommand)
{
    int16_t left = drv8701_clamp_command(leftCommand);
    int16_t right = drv8701_clamp_command(rightCommand);
    bool leftDirectionChanges =
        drv8701_direction_changes(gLeftCommand, left);
    bool rightDirectionChanges =
        drv8701_direction_changes(gRightCommand, right);

    /*
     * Never change PH while a channel is actively driving. First force EN
     * low and wait one complete PWM period, then change direction and apply
     * the new duty cycle.
     */
    if (leftDirectionChanges) {
        DL_TimerG_setCaptureCompareValue(
            DRV8701_PWM_INST, 0U, GPIO_DRV8701_PWM_C0_IDX);
    }
    if (rightDirectionChanges) {
        DL_TimerG_setCaptureCompareValue(
            DRV8701_PWM_INST, 0U, GPIO_DRV8701_PWM_C1_IDX);
    }
    if (leftDirectionChanges || rightDirectionChanges) {
        delay_cycles((CPUCLK_FREQ / DRV8701_PWM_FREQUENCY_HZ) + 1U);
    }

    if (left != 0) {
        drv8701_write_ph(DRV8701_DIR_LEFT_PH_PIN,
            drv8701_ph_high_for_command(
                left, DRV8701_LEFT_FORWARD_PH_HIGH));
    }
    if (right != 0) {
        drv8701_write_ph(DRV8701_DIR_RIGHT_PH_PIN,
            drv8701_ph_high_for_command(
                right, DRV8701_RIGHT_FORWARD_PH_HIGH));
    }

    DL_TimerG_setCaptureCompareValue(DRV8701_PWM_INST,
        drv8701_command_to_compare(left), GPIO_DRV8701_PWM_C0_IDX);
    DL_TimerG_setCaptureCompareValue(DRV8701_PWM_INST,
        drv8701_command_to_compare(right), GPIO_DRV8701_PWM_C1_IDX);

    gLeftCommand = left;
    gRightCommand = right;
}

int16_t DRV8701_getLeftCommand(void)
{
    return gLeftCommand;
}

int16_t DRV8701_getRightCommand(void)
{
    return gRightCommand;
}