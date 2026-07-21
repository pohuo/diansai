#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ModuleTests/06_tof_stp23l/test_tof_stp23l.h"

static uint32_t gNowMs;
static TofStp23lSample gSample;
static uint32_t gRemainingSamples;
static uint32_t gTimeoutCalls;

static uint32_t GetTimeMs(void)
{
    return gNowMs;
}

static bool PollSample(TofStp23lSample *sample)
{
    if (gRemainingSamples == 0U) {
        return false;
    }
    *sample = gSample;
    gRemainingSamples--;
    return true;
}

static void HandleTimeout(void)
{
    gTimeoutCalls++;
}

void HostTest_TofStp23l(void)
{
    const TofStp23lInterface interface = {
        GetTimeMs, PollSample, HandleTimeout
    };
    ModuleTestResult result = {0};
    uint32_t run;

    gNowMs = 0U;
    gTimeoutCalls = 0U;
    gSample.distance_mm = 1000U;
    gSample.valid = true;
    gRemainingSamples = TOF_STP23L_REQUIRED_SAMPLES;
    assert(TofStp23lTest_Attach(&interface));
    TofStp23lTest_Init();
    assert(TofStp23lTest_SetReference(1000U, 20U));

    for (run = 0U; run < 8U; run++) {
        result = TofStp23lTest_RunOnce();
    }
    assert(result.status == MODULE_TEST_PASSED);

    TofStp23lTest_Init();
    assert(TofStp23lTest_SetReference(1000U, 20U));
    gRemainingSamples = 0U;
    gNowMs = TOF_STP23L_TIMEOUT_MS + 1U;
    result = TofStp23lTest_RunOnce();
    assert(result.status == MODULE_TEST_FAILED);
    assert(result.error_code == TOF_STP23L_ERROR_TIMEOUT);
    assert(gTimeoutCalls == 1U);
}