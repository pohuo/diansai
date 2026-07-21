#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ModuleTests/05_line_sensor/test_line_sensor.h"

static uint16_t gChannels[LINE_SENSOR_CHANNEL_COUNT];

static bool ReadChannel(uint8_t channel, uint16_t *rawValue)
{
    if (channel >= LINE_SENSOR_CHANNEL_COUNT) {
        return false;
    }
    *rawValue = gChannels[channel];
    return true;
}

static void SetChannels(uint16_t channel0,
                        uint16_t channel1,
                        uint16_t channel2,
                        uint16_t channel3)
{
    gChannels[0] = channel0;
    gChannels[1] = channel1;
    gChannels[2] = channel2;
    gChannels[3] = channel3;
}

void HostTest_LineSensor(void)
{
    const LineSensorInterface interface = { ReadChannel };
    ModuleTestResult result;
    uint8_t blackMask;

    assert(LineSensorTest_Attach(&interface));
    LineSensorTest_Init(100U);

    SetChannels(900U, 800U, 100U, 200U);
    assert(LineSensorTest_CaptureSurface(LINE_SENSOR_SURFACE_BLACK));
    SetChannels(100U, 200U, 900U, 800U);
    assert(LineSensorTest_CaptureSurface(LINE_SENSOR_SURFACE_WHITE));
    result = LineSensorTest_RunOnce();
    assert(result.status == MODULE_TEST_PASSED);

    SetChannels(900U, 200U, 100U, 800U);
    assert(LineSensorTest_GetBinaryMask(&blackMask));
    assert(blackMask == 0x05U);

    LineSensorTest_Init(100U);
    SetChannels(500U, 500U, 500U, 500U);
    assert(LineSensorTest_CaptureSurface(LINE_SENSOR_SURFACE_BLACK));
    SetChannels(520U, 520U, 520U, 520U);
    assert(LineSensorTest_CaptureSurface(LINE_SENSOR_SURFACE_WHITE));
    result = LineSensorTest_RunOnce();
    assert(result.status == MODULE_TEST_FAILED);
    assert(result.error_code == LINE_SENSOR_ERROR_LOW_CONTRAST);
}