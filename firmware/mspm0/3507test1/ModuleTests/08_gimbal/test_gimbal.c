#include "test_gimbal.h"

#include <stddef.h>

static GimbalInterface gInterface;
static GimbalTestPhase gPhase;
static GimbalError gLastError;
static uint32_t gPhaseStartedAtMs;
static uint8_t gCompletedCycles;
static bool gAttached;
static bool gInitialized;
static bool gArmed;

static void GimbalDisableOutputs(void)
{
    if (gAttached) {
        gInterface.set_enabled(false);
    }
}

static bool GimbalGetPhaseTarget(GimbalTestPhase phase,
                                 GimbalAxis *axis,
                                 int32_t *targetMdeg,
                                 GimbalDirection *direction)
{
    switch (phase) {
        case GIMBAL_TEST_PHASE_PAN_POSITIVE:
            *axis = GIMBAL_AXIS_PAN;
            *targetMdeg = GIMBAL_TEST_PAN_MDEG;
            *direction = GIMBAL_DIRECTION_POSITIVE;
            return true;
        case GIMBAL_TEST_PHASE_PAN_CENTER_FROM_POSITIVE:
            *axis = GIMBAL_AXIS_PAN;
            *targetMdeg = 0;
            *direction = GIMBAL_DIRECTION_NEGATIVE;
            return true;
        case GIMBAL_TEST_PHASE_PAN_NEGATIVE:
            *axis = GIMBAL_AXIS_PAN;
            *targetMdeg = -GIMBAL_TEST_PAN_MDEG;
            *direction = GIMBAL_DIRECTION_NEGATIVE;
            return true;
        case GIMBAL_TEST_PHASE_PAN_CENTER_FROM_NEGATIVE:
            *axis = GIMBAL_AXIS_PAN;
            *targetMdeg = 0;
            *direction = GIMBAL_DIRECTION_POSITIVE;
            return true;
        case GIMBAL_TEST_PHASE_TILT_POSITIVE:
            *axis = GIMBAL_AXIS_TILT;
            *targetMdeg = GIMBAL_TEST_TILT_MDEG;
            *direction = GIMBAL_DIRECTION_POSITIVE;
            return true;
        case GIMBAL_TEST_PHASE_TILT_CENTER_FROM_POSITIVE:
            *axis = GIMBAL_AXIS_TILT;
            *targetMdeg = 0;
            *direction = GIMBAL_DIRECTION_NEGATIVE;
            return true;
        case GIMBAL_TEST_PHASE_TILT_NEGATIVE:
            *axis = GIMBAL_AXIS_TILT;
            *targetMdeg = -GIMBAL_TEST_TILT_MDEG;
            *direction = GIMBAL_DIRECTION_NEGATIVE;
            return true;
        case GIMBAL_TEST_PHASE_TILT_CENTER_FROM_NEGATIVE:
            *axis = GIMBAL_AXIS_TILT;
            *targetMdeg = 0;
            *direction = GIMBAL_DIRECTION_POSITIVE;
            return true;
        default:
            return false;
    }
}

static bool GimbalStartPhase(GimbalTestPhase phase)
{
    GimbalAxis axis;
    int32_t targetMdeg;
    GimbalDirection direction;

    if (!GimbalGetPhaseTarget(phase, &axis, &targetMdeg, &direction)) {
        return false;
    }
    if (gInterface.is_limit_active(axis, direction)) {
        gLastError = GIMBAL_ERROR_LIMIT_ACTIVE;
        return false;
    }
    if (!gInterface.command_angle_mdeg(axis, targetMdeg)) {
        gLastError = GIMBAL_ERROR_COMMAND_REJECTED;
        return false;
    }

    gPhase = phase;
    gPhaseStartedAtMs = gInterface.get_time_ms();
    return true;
}

static void GimbalAbort(GimbalError error)
{
    GimbalDisableOutputs();
    gArmed = false;
    gPhase = GIMBAL_TEST_PHASE_ABORTED;
    gLastError = error;
}

bool GimbalTest_Attach(const GimbalInterface *interface)
{
    if ((interface == NULL) || (interface->get_time_ms == NULL) ||
        (interface->is_estop_asserted == NULL) ||
        (interface->set_enabled == NULL) ||
        (interface->command_angle_mdeg == NULL) ||
        (interface->is_at_target == NULL) ||
        (interface->is_limit_active == NULL)) {
        gAttached = false;
        return false;
    }

    gInterface = *interface;
    gAttached = true;
    GimbalDisableOutputs();
    return true;
}

void GimbalTest_Init(void)
{
    gPhase = GIMBAL_TEST_PHASE_IDLE;
    gLastError = GIMBAL_ERROR_NONE;
    gPhaseStartedAtMs = 0U;
    gCompletedCycles = 0U;
    gArmed = false;
    gInitialized = gAttached;
    GimbalDisableOutputs();
    if (!gAttached) {
        gLastError = GIMBAL_ERROR_INVALID_INTERFACE;
    }
}

bool GimbalTest_Arm(void)
{
    if (!gInitialized || !gAttached ||
        (gCompletedCycles >= GIMBAL_REQUIRED_CYCLES)) {
        return false;
    }
    if (gInterface.is_estop_asserted()) {
        GimbalAbort(GIMBAL_ERROR_ESTOP_ACTIVE);
        return false;
    }

    gInterface.set_enabled(true);
    gArmed = true;
    gLastError = GIMBAL_ERROR_NONE;
    if (!GimbalStartPhase(GIMBAL_TEST_PHASE_PAN_POSITIVE)) {
        GimbalAbort(gLastError);
        return false;
    }
    return true;
}

ModuleTestResult GimbalTest_RunOnce(void)
{
    ModuleTestResult result = {
        MODULE_TEST_BLOCKED,
        (int32_t) gCompletedCycles,
        (int32_t) GIMBAL_REQUIRED_CYCLES,
        GIMBAL_ERROR_NOT_INITIALIZED
    };
    GimbalAxis axis;
    int32_t targetMdeg;
    GimbalDirection direction;
    GimbalTestPhase nextPhase;
    uint32_t nowMs;

    if (!gInitialized) {
        result.error_code = (uint32_t) gLastError;
        return result;
    }
    if (gCompletedCycles >= GIMBAL_REQUIRED_CYCLES) {
        result.status = MODULE_TEST_PASSED;
        result.error_code = GIMBAL_ERROR_NONE;
        return result;
    }
    if (!gArmed) {
        result.status = (gPhase == GIMBAL_TEST_PHASE_ABORTED)
                            ? MODULE_TEST_FAILED
                            : MODULE_TEST_RUNNING;
        result.error_code = (gPhase == GIMBAL_TEST_PHASE_ABORTED)
                                ? (uint32_t) gLastError
                                : GIMBAL_ERROR_ARM_REQUIRED;
        return result;
    }
    if (gInterface.is_estop_asserted()) {
        GimbalAbort(GIMBAL_ERROR_ESTOP_ACTIVE);
        result.status = MODULE_TEST_FAILED;
        result.error_code = GIMBAL_ERROR_ESTOP_ACTIVE;
        return result;
    }
    if (!GimbalGetPhaseTarget(gPhase, &axis, &targetMdeg, &direction)) {
        GimbalAbort(GIMBAL_ERROR_COMMAND_REJECTED);
        result.status = MODULE_TEST_FAILED;
        result.error_code = GIMBAL_ERROR_COMMAND_REJECTED;
        return result;
    }
    if (gInterface.is_limit_active(axis, direction)) {
        GimbalAbort(GIMBAL_ERROR_LIMIT_ACTIVE);
        result.status = MODULE_TEST_FAILED;
        result.error_code = GIMBAL_ERROR_LIMIT_ACTIVE;
        return result;
    }

    nowMs = gInterface.get_time_ms();
    if ((uint32_t) (nowMs - gPhaseStartedAtMs) >
        GIMBAL_TARGET_TIMEOUT_MS) {
        GimbalAbort(GIMBAL_ERROR_TARGET_TIMEOUT);
        result.status = MODULE_TEST_FAILED;
        result.error_code = GIMBAL_ERROR_TARGET_TIMEOUT;
        return result;
    }

    if (gInterface.is_at_target(axis, targetMdeg,
                                GIMBAL_TARGET_TOLERANCE_MDEG)) {
        if (gPhase == GIMBAL_TEST_PHASE_TILT_CENTER_FROM_NEGATIVE) {
            gCompletedCycles++;
            if (gCompletedCycles >= GIMBAL_REQUIRED_CYCLES) {
                GimbalDisableOutputs();
                gArmed = false;
                gPhase = GIMBAL_TEST_PHASE_COMPLETE;
                gLastError = GIMBAL_ERROR_NONE;
            } else if (!GimbalStartPhase(
                           GIMBAL_TEST_PHASE_PAN_POSITIVE)) {
                GimbalAbort(gLastError);
            }
        } else {
            nextPhase = (GimbalTestPhase) ((int) gPhase + 1);
            if (!GimbalStartPhase(nextPhase)) {
                GimbalAbort(gLastError);
            }
        }
    }

    result.measured_value = (int32_t) gCompletedCycles;
    result.status = (gCompletedCycles >= GIMBAL_REQUIRED_CYCLES)
                        ? MODULE_TEST_PASSED
                        : ((gPhase == GIMBAL_TEST_PHASE_ABORTED)
                               ? MODULE_TEST_FAILED
                               : MODULE_TEST_RUNNING);
    result.error_code = (uint32_t) gLastError;
    return result;
}

void GimbalTest_SafeStop(void)
{
    GimbalAbort(GIMBAL_ERROR_ABORTED);
}

GimbalTestPhase GimbalTest_GetPhase(void)
{
    return gPhase;
}

uint8_t GimbalTest_GetCompletedCycles(void)
{
    return gCompletedCycles;
}