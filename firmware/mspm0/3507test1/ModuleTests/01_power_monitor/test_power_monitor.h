#ifndef TEST_POWER_MONITOR_H
#define TEST_POWER_MONITOR_H

#include "power_monitor_interface.h"
#include "../common/module_test_contract.h"

#define POWER_MONITOR_DEFAULT_TOLERANCE_PERMILLE (30U)
#define POWER_MONITOR_DEFAULT_REQUIRED_SAMPLES    (32U)

typedef enum {
    POWER_MONITOR_ERROR_NONE = 0,
    POWER_MONITOR_ERROR_INVALID_CONFIG = 0x1001U,
    POWER_MONITOR_ERROR_INVALID_INTERFACE = 0x1002U,
    POWER_MONITOR_ERROR_NOT_INITIALIZED = 0x1003U,
    POWER_MONITOR_ERROR_ADC_READ = 0x1004U,
    POWER_MONITOR_ERROR_CALIBRATION_REQUIRED = 0x1005U,
    POWER_MONITOR_ERROR_OUT_OF_TOLERANCE = 0x1006U,
    POWER_MONITOR_ERROR_LOW_VOLTAGE = 0x1007U
} PowerMonitorError;

typedef struct {
    uint16_t adc_full_scale_counts;
    uint16_t adc_reference_mv;
    uint32_t divider_top_ohm;
    uint32_t divider_bottom_ohm;
    uint16_t low_voltage_mv;
    uint16_t recover_voltage_mv;
    uint16_t tolerance_permille;
    uint16_t required_samples;
    uint32_t low_voltage_debounce_ms;
} PowerMonitorConfig;

typedef struct {
    uint16_t raw_adc;
    uint32_t battery_mv;
    uint32_t filtered_mv;
    uint32_t reference_mv;
    uint32_t sample_count;
    bool low_voltage_latched;
} PowerMonitorReading;

bool PowerMonitorTest_Attach(const PowerMonitorConfig *config,
                             const PowerMonitorInterface *interface);
void PowerMonitorTest_Init(void);
bool PowerMonitorTest_SetReferenceMeasurement(uint32_t reference_mv);
ModuleTestResult PowerMonitorTest_RunOnce(void);
void PowerMonitorTest_SafeStop(void);
bool PowerMonitorTest_GetReading(PowerMonitorReading *reading);

#endif