#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ModuleTests/02_button_estop/test_button_estop.h"

static uint32_t gNowMs;
static bool gModePressed;
static bool gEstopAsserted;
static uint32_t gSafeCalls;

static uint32_t GetTimeMs(void)
{
    return gNowMs;
}

static bool ReadModePressed(void)
{
    return gModePressed;
}

static bool ReadEstopAsserted(void)
{
    return gEstopAsserted;
}

static void ForceActuatorsSafe(void)
{
    gSafeCalls++;
}

static ModuleTestResult SetStableModeState(bool pressed)
{
    ModuleTestResult result;

    gModePressed = pressed;
    result = ButtonEstopTest_RunOnce();
    gNowMs += BUTTON_ESTOP_DEBOUNCE_MS;
    result = ButtonEstopTest_RunOnce();
    return result;
}

void HostTest_ButtonEstop(void)
{
    const ButtonEstopInterface interface = {
        GetTimeMs, ReadModePressed, ReadEstopAsserted, ForceActuatorsSafe
    };
    ModuleTestResult result = {0};
    uint32_t press;

    gNowMs = 0U;
    gModePressed = false;
    gEstopAsserted = false;
    gSafeCalls = 0U;
    assert(ButtonEstopTest_Attach(&interface));
    ButtonEstopTest_Init();

    for (press = 0U; press < BUTTON_ESTOP_REQUIRED_PRESSES; press++) {
        result = SetStableModeState(true);
        result = SetStableModeState(false);
    }
    assert(ButtonEstopTest_GetPressCount() ==
           BUTTON_ESTOP_REQUIRED_PRESSES);
    assert(result.status == MODULE_TEST_RUNNING);

    gEstopAsserted = true;
    result = ButtonEstopTest_RunOnce();
    assert(result.status == MODULE_TEST_PASSED);
    assert(ButtonEstopTest_WasEstopObserved());
    assert(gSafeCalls == 1U);

    ButtonEstopTest_SafeStop();
    assert(gSafeCalls == 2U);
}