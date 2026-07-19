#include "test_encoder.h"

#define ENCODER_LEFT_FORWARD_BIT   (1U << 0)
#define ENCODER_LEFT_REVERSE_BIT   (1U << 1)
#define ENCODER_RIGHT_FORWARD_BIT  (1U << 2)
#define ENCODER_RIGHT_REVERSE_BIT  (1U << 3)
#define ENCODER_ALL_DIRECTIONS     (0x0FU)

static EncoderInterface gInterface;
static bool gAttached;
static bool gInitialized;
static int32_t gPreviousLeft;
static int32_t gPreviousRight;
static int32_t gLastLeftDelta;
static int32_t gLastRightDelta;
static uint32_t gLastSampleAtMs;
static uint8_t gObservedDirectionMask;
static EncoderTestError gLastError;

static uint32_t EncoderAbsDelta(int32_t value)
{
    if (value >= 0) {
        return (uint32_t) value;
    }
    return (uint32_t) (-(int64_t) value);
}

static uint8_t EncoderCountObservedDirections(uint8_t mask)
{
    uint8_t count = 0U;
    uint8_t bit;

    for (bit = 0U; bit < 4U; bit++) {
        if ((mask & (uint8_t) (1U << bit)) != 0U) {
            count++;
        }
    }
    return count;
}

bool EncoderTest_Attach(const EncoderInterface *interface)
{
    if ((interface == NULL) || (interface->get_time_ms == NULL) ||
        (interface->read_count == NULL)) {
        gAttached = false;
        return false;
    }

    gInterface = *interface;
    gAttached = true;
    return true;
}

void EncoderTest_Init(void)
{
    gInitialized = false;
    gObservedDirectionMask = 0U;
    gLastLeftDelta = 0;
    gLastRightDelta = 0;
    gLastError = ENCODER_TEST_ERROR_NONE;

    if (!gAttached) {
        gLastError = ENCODER_TEST_ERROR_INVALID_INTERFACE;
        return;
    }

    if (!gInterface.read_count(ENCODER_LEFT, &gPreviousLeft) ||
        !gInterface.read_count(ENCODER_RIGHT, &gPreviousRight)) {
        gLastError = ENCODER_TEST_ERROR_READ_FAILED;
        return;
    }

    gLastSampleAtMs = gInterface.get_time_ms();
    gInitialized = true;
}

ModuleTestResult EncoderTest_RunOnce(void)
{
    ModuleTestResult result = {
        MODULE_TEST_BLOCKED,
        (int32_t) EncoderCountObservedDirections(gObservedDirectionMask),
        4,
        ENCODER_TEST_ERROR_NOT_INITIALIZED
    };
    uint32_t nowMs;
    int32_t currentLeft;
    int32_t currentRight;

    if (!gInitialized) {
        result.error_code = (uint32_t) gLastError;
        return result;
    }

    nowMs = gInterface.get_time_ms();
    if ((uint32_t) (nowMs - gLastSampleAtMs) < ENCODER_TEST_SAMPLE_MS) {
        result.status = MODULE_TEST_RUNNING;
        result.error_code = ENCODER_TEST_ERROR_NONE;
        return result;
    }

    if (!gInterface.read_count(ENCODER_LEFT, &currentLeft) ||
        !gInterface.read_count(ENCODER_RIGHT, &currentRight)) {
        gLastError = ENCODER_TEST_ERROR_READ_FAILED;
        result.status = MODULE_TEST_FAILED;
        result.error_code = (uint32_t) gLastError;
        return result;
    }

    gLastLeftDelta =
        (int32_t) ((uint32_t) currentLeft - (uint32_t) gPreviousLeft);
    gLastRightDelta =
        (int32_t) ((uint32_t) currentRight - (uint32_t) gPreviousRight);
    gPreviousLeft = currentLeft;
    gPreviousRight = currentRight;
    gLastSampleAtMs = nowMs;

    if ((EncoderAbsDelta(gLastLeftDelta) >
            ENCODER_TEST_MAX_DELTA_PER_SAMPLE) ||
        (EncoderAbsDelta(gLastRightDelta) >
            ENCODER_TEST_MAX_DELTA_PER_SAMPLE)) {
        gLastError = ENCODER_TEST_ERROR_DELTA_IMPLAUSIBLE;
        result.status = MODULE_TEST_FAILED;
        result.error_code = (uint32_t) gLastError;
        return result;
    }

    if (gLastLeftDelta > 0) {
        gObservedDirectionMask |= ENCODER_LEFT_FORWARD_BIT;
    } else if (gLastLeftDelta < 0) {
        gObservedDirectionMask |= ENCODER_LEFT_REVERSE_BIT;
    }

    if (gLastRightDelta > 0) {
        gObservedDirectionMask |= ENCODER_RIGHT_FORWARD_BIT;
    } else if (gLastRightDelta < 0) {
        gObservedDirectionMask |= ENCODER_RIGHT_REVERSE_BIT;
    }

    result.measured_value =
        (int32_t) EncoderCountObservedDirections(gObservedDirectionMask);
    result.status = (gObservedDirectionMask == ENCODER_ALL_DIRECTIONS)
                        ? MODULE_TEST_PASSED
                        : MODULE_TEST_RUNNING;
    result.error_code = ENCODER_TEST_ERROR_NONE;
    return result;
}

void EncoderTest_SafeStop(void)
{
    if (gAttached && (gInterface.force_actuators_safe != NULL)) {
        gInterface.force_actuators_safe();
    }
    gInitialized = false;
}

void EncoderTest_GetLastDeltas(int32_t *left_delta, int32_t *right_delta)
{
    if (left_delta != NULL) {
        *left_delta = gLastLeftDelta;
    }
    if (right_delta != NULL) {
        *right_delta = gLastRightDelta;
    }
}

uint8_t EncoderTest_GetObservedDirectionMask(void)
{
    return gObservedDirectionMask;
}
