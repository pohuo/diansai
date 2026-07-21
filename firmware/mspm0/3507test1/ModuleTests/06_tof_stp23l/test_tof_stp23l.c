#include "test_tof_stp23l.h"

#include <limits.h>
#include <stddef.h>

static TofStp23lInterface gInterface;
static TofStp23lStats gStats;
static uint64_t gDistanceSumMm;
static uint32_t gLastSampleAtMs;
static uint16_t gReferenceMm;
static uint16_t gToleranceMm;
static TofStp23lError gLastError;
static bool gAttached;
static bool gInitialized;
static bool gTimeoutLatched;

static uint16_t TofAbsoluteDifference(uint16_t left, uint16_t right)
{
    return (left >= right) ? (uint16_t) (left - right)
                           : (uint16_t) (right - left);
}

bool TofStp23lTest_Attach(const TofStp23lInterface *interface)
{
    if ((interface == NULL) || (interface->get_time_ms == NULL) ||
        (interface->poll_sample == NULL) ||
        (interface->handle_timeout == NULL)) {
        gAttached = false;
        return false;
    }

    gInterface = *interface;
    gAttached = true;
    return true;
}

void TofStp23lTest_Init(void)
{
    gStats = (TofStp23lStats) {0};
    gStats.minimum_mm = UINT16_MAX;
    gDistanceSumMm = 0U;
    gReferenceMm = 0U;
    gToleranceMm = 0U;
    gTimeoutLatched = false;
    gInitialized = gAttached;
    gLastError = gAttached ? TOF_STP23L_ERROR_NONE
                           : TOF_STP23L_ERROR_INVALID_INTERFACE;
    if (gInitialized) {
        gLastSampleAtMs = gInterface.get_time_ms();
    }
}

bool TofStp23lTest_SetReference(uint16_t distanceMm,
                               uint16_t toleranceMm)
{
    if (!gInitialized ||
        (distanceMm < TOF_STP23L_MIN_DISTANCE_MM) ||
        (distanceMm > TOF_STP23L_MAX_DISTANCE_MM) ||
        (toleranceMm == 0U)) {
        return false;
    }

    gReferenceMm = distanceMm;
    gToleranceMm = toleranceMm;
    gStats.consecutive_good_samples = 0U;
    return true;
}

static void TofAcceptSample(const TofStp23lSample *sample, uint32_t nowMs)
{
    uint16_t errorMm;

    gLastSampleAtMs = nowMs;
    gTimeoutLatched = false;

    if (!sample->valid) {
        gStats.invalid_samples++;
        gStats.consecutive_good_samples = 0U;
        gLastError = TOF_STP23L_ERROR_INVALID_SAMPLE;
        return;
    }
    if ((sample->distance_mm < TOF_STP23L_MIN_DISTANCE_MM) ||
        (sample->distance_mm > TOF_STP23L_MAX_DISTANCE_MM)) {
        gStats.out_of_range_samples++;
        gStats.consecutive_good_samples = 0U;
        gLastError = TOF_STP23L_ERROR_OUT_OF_RANGE;
        return;
    }

    gStats.accepted_samples++;
    gDistanceSumMm += sample->distance_mm;
    if (sample->distance_mm < gStats.minimum_mm) {
        gStats.minimum_mm = sample->distance_mm;
    }
    if (sample->distance_mm > gStats.maximum_mm) {
        gStats.maximum_mm = sample->distance_mm;
    }
    gStats.average_mm =
        (uint32_t) (gDistanceSumMm / gStats.accepted_samples);

    if (gReferenceMm == 0U) {
        gStats.consecutive_good_samples = 0U;
        gLastError = TOF_STP23L_ERROR_REFERENCE_REQUIRED;
        return;
    }

    errorMm = TofAbsoluteDifference(sample->distance_mm, gReferenceMm);
    if (errorMm > gToleranceMm) {
        gStats.out_of_tolerance_samples++;
        gStats.consecutive_good_samples = 0U;
        gLastError = TOF_STP23L_ERROR_OUT_OF_TOLERANCE;
    } else {
        gStats.consecutive_good_samples++;
        gLastError = TOF_STP23L_ERROR_NONE;
    }
}

ModuleTestResult TofStp23lTest_RunOnce(void)
{
    ModuleTestResult result = {
        MODULE_TEST_BLOCKED, 0, 0, TOF_STP23L_ERROR_NOT_INITIALIZED
    };
    TofStp23lSample sample;
    uint8_t processed = 0U;
    uint32_t nowMs;

    if (!gInitialized) {
        result.error_code = (uint32_t) gLastError;
        return result;
    }

    nowMs = gInterface.get_time_ms();
    while ((processed < TOF_STP23L_MAX_SAMPLES_PER_RUN) &&
           gInterface.poll_sample(&sample)) {
        TofAcceptSample(&sample, nowMs);
        processed++;
        nowMs = gInterface.get_time_ms();
    }

    if (!gTimeoutLatched &&
        ((uint32_t) (nowMs - gLastSampleAtMs) > TOF_STP23L_TIMEOUT_MS)) {
        gStats.consecutive_good_samples = 0U;
        gStats.timeout_events++;
        gLastError = TOF_STP23L_ERROR_TIMEOUT;
        gTimeoutLatched = true;
        gInterface.handle_timeout();
    }

    result.measured_value =
        (gStats.average_mm > (uint32_t) INT32_MAX)
            ? INT32_MAX
            : (int32_t) gStats.average_mm;
    result.expected_value = (int32_t) gReferenceMm;
    result.error_code = (uint32_t) gLastError;

    if (gTimeoutLatched ||
        (gLastError == TOF_STP23L_ERROR_OUT_OF_RANGE)) {
        result.status = MODULE_TEST_FAILED;
    } else if (gReferenceMm == 0U) {
        result.status = MODULE_TEST_RUNNING;
        result.error_code = TOF_STP23L_ERROR_REFERENCE_REQUIRED;
    } else if (gStats.consecutive_good_samples >=
               TOF_STP23L_REQUIRED_SAMPLES) {
        result.status = MODULE_TEST_PASSED;
        result.error_code = TOF_STP23L_ERROR_NONE;
    } else {
        result.status = MODULE_TEST_RUNNING;
    }
    return result;
}

void TofStp23lTest_SafeStop(void)
{
    if (gAttached) {
        gInterface.handle_timeout();
    }
    gInitialized = false;
}

void TofStp23lTest_GetStats(TofStp23lStats *stats)
{
    if (stats != NULL) {
        *stats = gStats;
    }
}