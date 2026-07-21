#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ModuleTests/04_encoder/test_encoder.h"

static uint32_t gNowMs;
static int32_t gCounts[2];
static uint32_t gSafeCalls;

static uint32_t GetTimeMs(void)
{
    return gNowMs;
}

static bool ReadCount(EncoderChannel channel, int32_t *count)
{
    *count = gCounts[(uint32_t) channel];
    return true;
}

static void ForceActuatorsSafe(void)
{
    gSafeCalls++;
}

void HostTest_Encoder(void)
{
    const EncoderInterface interface = {
        GetTimeMs, ReadCount, ForceActuatorsSafe
    };
    ModuleTestResult result;
    int32_t leftDelta;
    int32_t rightDelta;

    gNowMs = 0U;
    gCounts[ENCODER_LEFT] = 0;
    gCounts[ENCODER_RIGHT] = 0;
    gSafeCalls = 0U;
    assert(EncoderTest_Attach(&interface));
    EncoderTest_Init();

    gCounts[ENCODER_LEFT] = 10;
    gCounts[ENCODER_RIGHT] = 20;
    gNowMs += ENCODER_TEST_SAMPLE_MS;
    result = EncoderTest_RunOnce();
    assert(result.status == MODULE_TEST_RUNNING);

    gCounts[ENCODER_LEFT] = -10;
    gCounts[ENCODER_RIGHT] = -20;
    gNowMs += ENCODER_TEST_SAMPLE_MS;
    result = EncoderTest_RunOnce();
    assert(result.status == MODULE_TEST_PASSED);
    assert(EncoderTest_GetObservedDirectionMask() == 0x0FU);
    EncoderTest_GetLastDeltas(&leftDelta, &rightDelta);
    assert(leftDelta == -20);
    assert(rightDelta == -40);

    gCounts[ENCODER_LEFT] = 0;
    gCounts[ENCODER_RIGHT] = 0;
    EncoderTest_Init();
    gCounts[ENCODER_LEFT] =
        (int32_t) ENCODER_TEST_MAX_DELTA_PER_SAMPLE + 1;
    gNowMs += ENCODER_TEST_SAMPLE_MS;
    result = EncoderTest_RunOnce();
    assert(result.status == MODULE_TEST_FAILED);
    assert(result.error_code == ENCODER_TEST_ERROR_DELTA_IMPLAUSIBLE);

    EncoderTest_SafeStop();
    assert(gSafeCalls == 1U);
}