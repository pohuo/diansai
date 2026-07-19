#include "test_motor_tb6612.h"

static MotorTb6612Interface gInterface;
static bool gAttached;
static bool gInitialized;
static bool gArmed;
static uint8_t gCompletedCycles;
static uint32_t gPhaseStartedAtMs;
static MotorTb6612TestPhase gPhase;
static MotorTb6612Error gLastError;

static void MotorApplySafeOutputs(void)
{
    if (!gAttached) {
        return;
    }

    gInterface.set_motor_output(
        MOTOR_TB6612_LEFT, MOTOR_TB6612_STOP, 0U);
    gInterface.set_motor_output(
        MOTOR_TB6612_RIGHT, MOTOR_TB6612_STOP, 0U);
    gInterface.set_standby(false);
}

static void MotorStopForDirectionChange(uint32_t nowMs,
                                        MotorTb6612TestPhase dwellPhase)
{
    gInterface.set_motor_output(
        MOTOR_TB6612_LEFT, MOTOR_TB6612_STOP, 0U);
    gInterface.set_motor_output(
        MOTOR_TB6612_RIGHT, MOTOR_TB6612_STOP, 0U);
    gPhase = dwellPhase;
    gPhaseStartedAtMs = nowMs;
}

static void MotorStartPhase(MotorTb6612TestPhase phase, uint32_t nowMs)
{
    gInterface.set_standby(true);
    gInterface.set_motor_output(
        MOTOR_TB6612_LEFT, MOTOR_TB6612_STOP, 0U);
    gInterface.set_motor_output(
        MOTOR_TB6612_RIGHT, MOTOR_TB6612_STOP, 0U);

    if (phase == MOTOR_TEST_PHASE_LEFT_FORWARD) {
        gInterface.set_motor_output(MOTOR_TB6612_LEFT,
            MOTOR_TB6612_FORWARD, MOTOR_TB6612_TEST_PWM_PERMILLE);
    } else if (phase == MOTOR_TEST_PHASE_LEFT_REVERSE) {
        gInterface.set_motor_output(MOTOR_TB6612_LEFT,
            MOTOR_TB6612_REVERSE, MOTOR_TB6612_TEST_PWM_PERMILLE);
    } else if (phase == MOTOR_TEST_PHASE_RIGHT_FORWARD) {
        gInterface.set_motor_output(MOTOR_TB6612_RIGHT,
            MOTOR_TB6612_FORWARD, MOTOR_TB6612_TEST_PWM_PERMILLE);
    } else if (phase == MOTOR_TEST_PHASE_RIGHT_REVERSE) {
        gInterface.set_motor_output(MOTOR_TB6612_RIGHT,
            MOTOR_TB6612_REVERSE, MOTOR_TB6612_TEST_PWM_PERMILLE);
    }

    gPhase = phase;
    gPhaseStartedAtMs = nowMs;
}

bool MotorTb6612Test_Attach(const MotorTb6612Interface *interface)
{
    if ((interface == NULL) || (interface->get_time_ms == NULL) ||
        (interface->is_estop_asserted == NULL) ||
        (interface->set_standby == NULL) ||
        (interface->set_motor_output == NULL)) {
        gAttached = false;
        return false;
    }

    gInterface = *interface;
    gAttached = true;
    MotorApplySafeOutputs();
    return true;
}

void MotorTb6612Test_Init(void)
{
    gInitialized = false;
    gArmed = false;
    gCompletedCycles = 0U;
    gPhase = MOTOR_TEST_PHASE_IDLE;
    gLastError = MOTOR_TB6612_ERROR_NONE;

    if (!gAttached) {
        gLastError = MOTOR_TB6612_ERROR_INVALID_INTERFACE;
        return;
    }

    MotorApplySafeOutputs();
    gInitialized = true;
}

bool MotorTb6612Test_Arm(void)
{
    if (!gInitialized || !gAttached ||
        (gCompletedCycles >= MOTOR_TB6612_REQUIRED_CYCLES)) {
        return false;
    }

    if (gInterface.is_estop_asserted()) {
        gLastError = MOTOR_TB6612_ERROR_ESTOP_ACTIVE;
        MotorApplySafeOutputs();
        return false;
    }

    gArmed = true;
    gLastError = MOTOR_TB6612_ERROR_NONE;
    MotorStartPhase(
        MOTOR_TEST_PHASE_LEFT_FORWARD, gInterface.get_time_ms());
    return true;
}

ModuleTestResult MotorTb6612Test_RunOnce(void)
{
    ModuleTestResult result = {
        MODULE_TEST_BLOCKED,
        (int32_t) gCompletedCycles,
        (int32_t) MOTOR_TB6612_REQUIRED_CYCLES,
        MOTOR_TB6612_ERROR_NOT_INITIALIZED
    };
    uint32_t nowMs;
    uint32_t elapsedMs;

    if (!gInitialized) {
        result.error_code = gAttached ? MOTOR_TB6612_ERROR_NOT_INITIALIZED
                                      : MOTOR_TB6612_ERROR_INVALID_INTERFACE;
        return result;
    }

    if (gCompletedCycles >= MOTOR_TB6612_REQUIRED_CYCLES) {
        result.status = MODULE_TEST_PASSED;
        result.error_code = MOTOR_TB6612_ERROR_NONE;
        return result;
    }

    if (!gArmed) {
        result.status = MODULE_TEST_RUNNING;
        result.error_code = (uint32_t) MOTOR_TB6612_ERROR_ARM_REQUIRED;
        return result;
    }

    if (gInterface.is_estop_asserted()) {
        MotorApplySafeOutputs();
        gArmed = false;
        gPhase = MOTOR_TEST_PHASE_ABORTED;
        gLastError = MOTOR_TB6612_ERROR_ESTOP_ACTIVE;
        result.status = MODULE_TEST_FAILED;
        result.error_code = (uint32_t) gLastError;
        return result;
    }

    nowMs = gInterface.get_time_ms();
    elapsedMs = (uint32_t) (nowMs - gPhaseStartedAtMs);

    switch (gPhase) {
        case MOTOR_TEST_PHASE_LEFT_FORWARD:
            if (elapsedMs >= MOTOR_TB6612_MOTION_MS) {
                MotorStopForDirectionChange(nowMs, MOTOR_TEST_PHASE_DWELL_1);
            }
            break;
        case MOTOR_TEST_PHASE_DWELL_1:
            if (elapsedMs >= MOTOR_TB6612_DIRECTION_DWELL_MS) {
                MotorStartPhase(MOTOR_TEST_PHASE_LEFT_REVERSE, nowMs);
            }
            break;
        case MOTOR_TEST_PHASE_LEFT_REVERSE:
            if (elapsedMs >= MOTOR_TB6612_MOTION_MS) {
                MotorStopForDirectionChange(nowMs, MOTOR_TEST_PHASE_DWELL_2);
            }
            break;
        case MOTOR_TEST_PHASE_DWELL_2:
            if (elapsedMs >= MOTOR_TB6612_DIRECTION_DWELL_MS) {
                MotorStartPhase(MOTOR_TEST_PHASE_RIGHT_FORWARD, nowMs);
            }
            break;
        case MOTOR_TEST_PHASE_RIGHT_FORWARD:
            if (elapsedMs >= MOTOR_TB6612_MOTION_MS) {
                MotorStopForDirectionChange(nowMs, MOTOR_TEST_PHASE_DWELL_3);
            }
            break;
        case MOTOR_TEST_PHASE_DWELL_3:
            if (elapsedMs >= MOTOR_TB6612_DIRECTION_DWELL_MS) {
                MotorStartPhase(MOTOR_TEST_PHASE_RIGHT_REVERSE, nowMs);
            }
            break;
        case MOTOR_TEST_PHASE_RIGHT_REVERSE:
            if (elapsedMs >= MOTOR_TB6612_MOTION_MS) {
                MotorApplySafeOutputs();
                gArmed = false;
                gCompletedCycles++;
                gPhase = (gCompletedCycles >= MOTOR_TB6612_REQUIRED_CYCLES)
                             ? MOTOR_TEST_PHASE_COMPLETE
                             : MOTOR_TEST_PHASE_IDLE;
            }
            break;
        default:
            break;
    }

    result.measured_value = (int32_t) gCompletedCycles;
    result.status = (gCompletedCycles >= MOTOR_TB6612_REQUIRED_CYCLES)
                        ? MODULE_TEST_PASSED
                        : MODULE_TEST_RUNNING;
    result.error_code = MOTOR_TB6612_ERROR_NONE;
    return result;
}

void MotorTb6612Test_SafeStop(void)
{
    MotorApplySafeOutputs();
    gArmed = false;
    gPhase = MOTOR_TEST_PHASE_ABORTED;
    gLastError = MOTOR_TB6612_ERROR_ABORTED;
}

MotorTb6612TestPhase MotorTb6612Test_GetPhase(void)
{
    return gPhase;
}

uint8_t MotorTb6612Test_GetCompletedCycles(void)
{
    return gCompletedCycles;
}
