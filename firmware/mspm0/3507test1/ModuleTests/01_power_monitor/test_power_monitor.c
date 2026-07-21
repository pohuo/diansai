#include "test_power_monitor.h"

#include <limits.h>
#include <stddef.h>

static PowerMonitorConfig gConfig;
static PowerMonitorInterface gInterface;
static PowerMonitorReading gReading;
static PowerMonitorError gLastError;
static uint32_t gLowVoltageStartedAtMs;
static bool gAttached;
static bool gInitialized;
static bool gLowVoltageTiming;

static bool PowerConfigIsValid(const PowerMonitorConfig *config)
{
    return (config != NULL) && (config->adc_full_scale_counts > 0U) &&
           (config->adc_reference_mv > 0U) &&
           (config->divider_bottom_ohm > 0U) &&
           (config->recover_voltage_mv > config->low_voltage_mv) &&
           (config->tolerance_permille > 0U) &&
           (config->required_samples > 0U);
}

static uint32_t PowerConvertToMillivolts(uint16_t rawAdc)
{
    uint64_t numerator =
        (uint64_t) rawAdc * gConfig.adc_reference_mv *
        ((uint64_t) gConfig.divider_top_ohm +
         gConfig.divider_bottom_ohm);
    uint64_t denominator = (uint64_t) gConfig.adc_full_scale_counts *
                           gConfig.divider_bottom_ohm;

    return (uint32_t) ((numerator + (denominator / 2U)) / denominator);
}

static uint32_t PowerAbsoluteDifference(uint32_t left, uint32_t right)
{
    return (left >= right) ? (left - right) : (right - left);
}

bool PowerMonitorTest_Attach(const PowerMonitorConfig *config,
                             const PowerMonitorInterface *interface)
{
    if (!PowerConfigIsValid(config)) {
        gAttached = false;
        gLastError = POWER_MONITOR_ERROR_INVALID_CONFIG;
        return false;
    }
    if ((interface == NULL) || (interface->get_time_ms == NULL) ||
        (interface->read_battery_adc == NULL) ||
        (interface->handle_low_voltage == NULL)) {
        gAttached = false;
        gLastError = POWER_MONITOR_ERROR_INVALID_INTERFACE;
        return false;
    }

    gConfig = *config;
    gInterface = *interface;
    gAttached = true;
    gLastError = POWER_MONITOR_ERROR_NONE;
    return true;
}

void PowerMonitorTest_Init(void)
{
    gReading = (PowerMonitorReading) {0};
    gLowVoltageStartedAtMs = 0U;
    gLowVoltageTiming = false;
    gInitialized = gAttached;
    gLastError = gAttached ? POWER_MONITOR_ERROR_NONE
                           : POWER_MONITOR_ERROR_INVALID_INTERFACE;
}

bool PowerMonitorTest_SetReferenceMeasurement(uint32_t referenceMv)
{
    if (!gInitialized || (referenceMv == 0U)) {
        return false;
    }

    gReading.reference_mv = referenceMv;
    gReading.sample_count = 0U;
    return true;
}

ModuleTestResult PowerMonitorTest_RunOnce(void)
{
    ModuleTestResult result = {
        MODULE_TEST_BLOCKED, 0, 0, POWER_MONITOR_ERROR_NOT_INITIALIZED
    };
    uint16_t rawAdc;
    uint32_t nowMs;
    uint32_t allowedErrorMv;
    uint32_t errorMv;

    if (!gInitialized) {
        result.error_code = (uint32_t) gLastError;
        return result;
    }

    if (!gInterface.read_battery_adc(&rawAdc)) {
        result.status = MODULE_TEST_FAILED;
        result.error_code = POWER_MONITOR_ERROR_ADC_READ;
        return result;
    }

    gReading.raw_adc = rawAdc;
    gReading.battery_mv = PowerConvertToMillivolts(rawAdc);
    if (gReading.sample_count == 0U) {
        gReading.filtered_mv = gReading.battery_mv;
    } else {
        gReading.filtered_mv =
            ((gReading.filtered_mv * 7U) + gReading.battery_mv + 4U) / 8U;
    }
    gReading.sample_count++;
    nowMs = gInterface.get_time_ms();

    if (gReading.filtered_mv <= gConfig.low_voltage_mv) {
        if (!gLowVoltageTiming) {
            gLowVoltageTiming = true;
            gLowVoltageStartedAtMs = nowMs;
        } else if (!gReading.low_voltage_latched &&
                   ((uint32_t) (nowMs - gLowVoltageStartedAtMs) >=
                    gConfig.low_voltage_debounce_ms)) {
            gReading.low_voltage_latched = true;
            gLastError = POWER_MONITOR_ERROR_LOW_VOLTAGE;
            gInterface.handle_low_voltage();
        }
    } else if (gReading.filtered_mv >= gConfig.recover_voltage_mv) {
        gLowVoltageTiming = false;
    }

    result.measured_value = (gReading.filtered_mv > (uint32_t) INT32_MAX)
                                ? INT32_MAX
                                : (int32_t) gReading.filtered_mv;
    result.expected_value = (gReading.reference_mv > (uint32_t) INT32_MAX)
                                ? INT32_MAX
                                : (int32_t) gReading.reference_mv;

    if (gReading.low_voltage_latched) {
        result.status = MODULE_TEST_FAILED;
        result.error_code = POWER_MONITOR_ERROR_LOW_VOLTAGE;
        return result;
    }
    if (gReading.reference_mv == 0U) {
        result.status = MODULE_TEST_RUNNING;
        result.error_code = POWER_MONITOR_ERROR_CALIBRATION_REQUIRED;
        return result;
    }

    allowedErrorMv = (uint32_t) (((uint64_t) gReading.reference_mv *
                                  gConfig.tolerance_permille + 999U) /
                                 1000U);
    errorMv = PowerAbsoluteDifference(gReading.filtered_mv,
                                      gReading.reference_mv);
    if (errorMv > allowedErrorMv) {
        gLastError = POWER_MONITOR_ERROR_OUT_OF_TOLERANCE;
        result.status = MODULE_TEST_FAILED;
        result.error_code = (uint32_t) gLastError;
    } else if (gReading.sample_count >= gConfig.required_samples) {
        gLastError = POWER_MONITOR_ERROR_NONE;
        result.status = MODULE_TEST_PASSED;
        result.error_code = POWER_MONITOR_ERROR_NONE;
    } else {
        gLastError = POWER_MONITOR_ERROR_NONE;
        result.status = MODULE_TEST_RUNNING;
        result.error_code = POWER_MONITOR_ERROR_NONE;
    }
    return result;
}

void PowerMonitorTest_SafeStop(void)
{
    if (gAttached) {
        gInterface.handle_low_voltage();
    }
    gInitialized = false;
}

bool PowerMonitorTest_GetReading(PowerMonitorReading *reading)
{
    if ((reading == NULL) || !gInitialized) {
        return false;
    }
    *reading = gReading;
    return true;
}