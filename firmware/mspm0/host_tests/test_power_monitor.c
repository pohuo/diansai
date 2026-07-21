#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ModuleTests/01_power_monitor/test_power_monitor.h"

static uint32_t gNowMs;
static uint16_t gRawAdc;
static uint32_t gSafetyCalls;

static uint32_t GetTimeMs(void)
{
    return gNowMs;
}

static bool ReadBatteryAdc(uint16_t *rawAdc)
{
    *rawAdc = gRawAdc;
    return true;
}

static void HandleLowVoltage(void)
{
    gSafetyCalls++;
}

void HostTest_PowerMonitor(void)
{
    const PowerMonitorConfig config = {
        4095U, 3300U, 30000U, 10000U,
        10000U, 10500U, 30U, 32U, 50U
    };
    const PowerMonitorInterface interface = {
        GetTimeMs, ReadBatteryAdc, HandleLowVoltage
    };
    ModuleTestResult result = {0};
    uint32_t sample;

    gNowMs = 0U;
    gSafetyCalls = 0U;
    gRawAdc = 3723U;
    assert(PowerMonitorTest_Attach(&config, &interface));
    PowerMonitorTest_Init();
    assert(PowerMonitorTest_SetReferenceMeasurement(12000U));

    for (sample = 0U; sample < 32U; sample++) {
        result = PowerMonitorTest_RunOnce();
        gNowMs++;
    }
    assert(result.status == MODULE_TEST_PASSED);

    PowerMonitorTest_Init();
    gRawAdc = 2792U;
    gNowMs = 100U;
    result = PowerMonitorTest_RunOnce();
    assert(result.status == MODULE_TEST_RUNNING);
    gNowMs = 151U;
    result = PowerMonitorTest_RunOnce();
    assert(result.status == MODULE_TEST_FAILED);
    assert(result.error_code == POWER_MONITOR_ERROR_LOW_VOLTAGE);
    assert(gSafetyCalls == 1U);
}