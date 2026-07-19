#include "test_line_sensor.h"

static LineSensorInterface gInterface;
static bool gAttached;
static bool gInitialized;
static bool gHasBlackSample;
static bool gHasWhiteSample;
static uint16_t gMinimumContrast;
static uint16_t gBlackValues[LINE_SENSOR_CHANNEL_COUNT];
static uint16_t gWhiteValues[LINE_SENSOR_CHANNEL_COUNT];

static bool LineSensorReadAll(uint16_t values[LINE_SENSOR_CHANNEL_COUNT])
{
    uint8_t channel;

    for (channel = 0U; channel < LINE_SENSOR_CHANNEL_COUNT; channel++) {
        if (!gInterface.read_channel(channel, &values[channel])) {
            return false;
        }
    }
    return true;
}

static uint16_t LineSensorDifference(uint16_t left, uint16_t right)
{
    return (left >= right) ? (uint16_t) (left - right)
                           : (uint16_t) (right - left);
}

static uint16_t LineSensorThreshold(uint8_t channel)
{
    return (uint16_t) (((uint32_t) gBlackValues[channel] +
                        (uint32_t) gWhiteValues[channel]) /
                       2U);
}

bool LineSensorTest_Attach(const LineSensorInterface *interface)
{
    if ((interface == NULL) || (interface->read_channel == NULL)) {
        gAttached = false;
        return false;
    }

    gInterface = *interface;
    gAttached = true;
    return true;
}

void LineSensorTest_Init(uint16_t minimum_contrast)
{
    uint8_t channel;

    gInitialized = gAttached;
    gHasBlackSample = false;
    gHasWhiteSample = false;
    gMinimumContrast = (minimum_contrast == 0U) ? 1U : minimum_contrast;

    for (channel = 0U; channel < LINE_SENSOR_CHANNEL_COUNT; channel++) {
        gBlackValues[channel] = 0U;
        gWhiteValues[channel] = 0U;
    }
}

bool LineSensorTest_CaptureSurface(LineSensorSurface surface)
{
    uint16_t values[LINE_SENSOR_CHANNEL_COUNT];
    uint8_t channel;

    if (!gInitialized || !LineSensorReadAll(values)) {
        return false;
    }

    for (channel = 0U; channel < LINE_SENSOR_CHANNEL_COUNT; channel++) {
        if (surface == LINE_SENSOR_SURFACE_BLACK) {
            gBlackValues[channel] = values[channel];
        } else {
            gWhiteValues[channel] = values[channel];
        }
    }

    if (surface == LINE_SENSOR_SURFACE_BLACK) {
        gHasBlackSample = true;
    } else {
        gHasWhiteSample = true;
    }
    return true;
}

ModuleTestResult LineSensorTest_RunOnce(void)
{
    ModuleTestResult result = {
        MODULE_TEST_BLOCKED,
        0,
        (int32_t) LINE_SENSOR_CHANNEL_COUNT,
        LINE_SENSOR_ERROR_NOT_INITIALIZED
    };
    uint8_t channel;
    uint8_t validChannels = 0U;

    if (!gInitialized) {
        result.error_code = gAttached ? LINE_SENSOR_ERROR_NOT_INITIALIZED
                                      : LINE_SENSOR_ERROR_INVALID_INTERFACE;
        return result;
    }
    if (!gHasBlackSample) {
        result.status = MODULE_TEST_RUNNING;
        result.error_code = LINE_SENSOR_ERROR_NEED_BLACK_SAMPLE;
        return result;
    }
    if (!gHasWhiteSample) {
        result.status = MODULE_TEST_RUNNING;
        result.error_code = LINE_SENSOR_ERROR_NEED_WHITE_SAMPLE;
        return result;
    }

    for (channel = 0U; channel < LINE_SENSOR_CHANNEL_COUNT; channel++) {
        if (LineSensorDifference(gBlackValues[channel],
                                 gWhiteValues[channel]) >= gMinimumContrast) {
            validChannels++;
        }
    }

    result.measured_value = (int32_t) validChannels;
    result.status = (validChannels == LINE_SENSOR_CHANNEL_COUNT)
                        ? MODULE_TEST_PASSED
                        : MODULE_TEST_FAILED;
    result.error_code = (validChannels == LINE_SENSOR_CHANNEL_COUNT)
                            ? LINE_SENSOR_ERROR_NONE
                            : LINE_SENSOR_ERROR_LOW_CONTRAST;
    return result;
}

void LineSensorTest_SafeStop(void)
{
    gInitialized = false;
}

bool LineSensorTest_GetBinaryMask(uint8_t *black_mask)
{
    uint16_t current[LINE_SENSOR_CHANNEL_COUNT];
    uint8_t channel;
    uint8_t mask = 0U;

    if ((black_mask == NULL) || !gInitialized || !gHasBlackSample ||
        !gHasWhiteSample || !LineSensorReadAll(current)) {
        return false;
    }

    for (channel = 0U; channel < LINE_SENSOR_CHANNEL_COUNT; channel++) {
        uint16_t threshold = LineSensorThreshold(channel);
        bool isBlack = (gBlackValues[channel] >= gWhiteValues[channel])
                           ? (current[channel] >= threshold)
                           : (current[channel] <= threshold);
        if (isBlack) {
            mask |= (uint8_t) (1U << channel);
        }
    }

    *black_mask = mask;
    return true;
}

bool LineSensorTest_GetCalibration(uint8_t channel,
                                   uint16_t *black_value,
                                   uint16_t *white_value,
                                   uint16_t *threshold)
{
    if ((channel >= LINE_SENSOR_CHANNEL_COUNT) || !gHasBlackSample ||
        !gHasWhiteSample) {
        return false;
    }

    if (black_value != NULL) {
        *black_value = gBlackValues[channel];
    }
    if (white_value != NULL) {
        *white_value = gWhiteValues[channel];
    }
    if (threshold != NULL) {
        *threshold = LineSensorThreshold(channel);
    }
    return true;
}
