#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ModuleTests/08_gimbal/test_gimbal.h"

static uint32_t gNowMs;
static bool gEstopAsserted;
static bool gEnabled;
static bool gTargetReached;
static GimbalAxis gCommandAxis;
static int32_t gCommandTargetMdeg;

static uint32_t GetTimeMs(void)
{
    return gNowMs;
}

static bool IsEstopAsserted(void)
{
    return gEstopAsserted;
}

static void SetEnabled(bool enabled)
{
    gEnabled = enabled;
}

static bool CommandAngle(GimbalAxis axis, int32_t targetMdeg)
{
    gCommandAxis = axis;
    gCommandTargetMdeg = targetMdeg;
    return true;
}

static bool IsAtTarget(GimbalAxis axis,
                       int32_t targetMdeg,
                       uint32_t toleranceMdeg)
{
    (void) toleranceMdeg;
    return gTargetReached && (axis == gCommandAxis) &&
           (targetMdeg == gCommandTargetMdeg);
}

static bool IsLimitActive(GimbalAxis axis, GimbalDirection direction)
{
    (void) axis;
    (void) direction;
    return false;
}

void HostTest_Gimbal(void)
{
    const GimbalInterface interface = {
        GetTimeMs, IsEstopAsserted, SetEnabled,
        CommandAngle, IsAtTarget, IsLimitActive
    };
    ModuleTestResult result = {0};
    uint32_t step;

    gNowMs = 0U;
    gEstopAsserted = false;
    gEnabled = false;
    gTargetReached = true;
    assert(GimbalTest_Attach(&interface));
    GimbalTest_Init();
    assert(!gEnabled);
    assert(GimbalTest_Arm());
    assert(gEnabled);

    for (step = 0U; step < 80U; step++) {
        result = GimbalTest_RunOnce();
    }
    assert(result.status == MODULE_TEST_PASSED);
    assert(!gEnabled);

    GimbalTest_Init();
    gTargetReached = false;
    assert(GimbalTest_Arm());
    gNowMs += GIMBAL_TARGET_TIMEOUT_MS + 1U;
    result = GimbalTest_RunOnce();
    assert(result.status == MODULE_TEST_FAILED);
    assert(result.error_code == GIMBAL_ERROR_TARGET_TIMEOUT);
    assert(!gEnabled);

    GimbalTest_Init();
    gTargetReached = true;
    assert(GimbalTest_Arm());
    gEstopAsserted = true;
    result = GimbalTest_RunOnce();
    assert(result.status == MODULE_TEST_FAILED);
    assert(result.error_code == GIMBAL_ERROR_ESTOP_ACTIVE);
    assert(!gEnabled);
}