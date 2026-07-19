#include "test_button_estop.h"

static ButtonEstopInterface gInterface;
static bool gAttached;
static bool gInitialized;
static bool gLastRawPressed;
static bool gStablePressed;
static bool gEstopObserved;
static bool gSafeStopObserved;
static uint16_t gPressCount;
static uint32_t gRawChangedAtMs;

bool ButtonEstopTest_Attach(const ButtonEstopInterface *interface)
{
    if ((interface == NULL) || (interface->get_time_ms == NULL) ||
        (interface->read_mode_pressed == NULL) ||
        (interface->read_estop_asserted == NULL) ||
        (interface->force_actuators_safe == NULL)) {
        gAttached = false;
        return false;
    }

    gInterface = *interface;
    gAttached = true;
    return true;
}

void ButtonEstopTest_Init(void)
{
    gInitialized = false;
    gPressCount = 0U;
    gEstopObserved = false;
    gSafeStopObserved = false;

    if (!gAttached) {
        return;
    }

    gLastRawPressed = gInterface.read_mode_pressed();
    gStablePressed = gLastRawPressed;
    gRawChangedAtMs = gInterface.get_time_ms();
    gInitialized = true;
}

ModuleTestResult ButtonEstopTest_RunOnce(void)
{
    ModuleTestResult result = {
        MODULE_TEST_BLOCKED,
        (int32_t) gPressCount,
        (int32_t) BUTTON_ESTOP_REQUIRED_PRESSES,
        BUTTON_ESTOP_ERROR_NOT_INITIALIZED
    };
    uint32_t nowMs;
    bool rawPressed;

    if (!gInitialized) {
        result.error_code = gAttached ? BUTTON_ESTOP_ERROR_NOT_INITIALIZED
                                      : BUTTON_ESTOP_ERROR_INVALID_INTERFACE;
        return result;
    }

    nowMs = gInterface.get_time_ms();

    if (gInterface.read_estop_asserted()) {
        gInterface.force_actuators_safe();
        gEstopObserved = true;
        gSafeStopObserved = true;
    }

    rawPressed = gInterface.read_mode_pressed();
    if (rawPressed != gLastRawPressed) {
        gLastRawPressed = rawPressed;
        gRawChangedAtMs = nowMs;
    }

    if ((rawPressed != gStablePressed) &&
        ((uint32_t) (nowMs - gRawChangedAtMs) >= BUTTON_ESTOP_DEBOUNCE_MS)) {
        gStablePressed = rawPressed;
        if (gStablePressed && (gPressCount < UINT16_MAX)) {
            gPressCount++;
        }
    }

    result.measured_value = (int32_t) gPressCount;
    result.error_code = BUTTON_ESTOP_ERROR_NONE;
    result.status = ((gPressCount >= BUTTON_ESTOP_REQUIRED_PRESSES) &&
                        gEstopObserved && gSafeStopObserved)
                        ? MODULE_TEST_PASSED
                        : MODULE_TEST_RUNNING;
    return result;
}

void ButtonEstopTest_SafeStop(void)
{
    if (gAttached) {
        gInterface.force_actuators_safe();
    }
    gInitialized = false;
}

uint16_t ButtonEstopTest_GetPressCount(void)
{
    return gPressCount;
}

bool ButtonEstopTest_WasEstopObserved(void)
{
    return gEstopObserved;
}
