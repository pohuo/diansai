#ifndef TEST_LINE_SENSOR_H
#define TEST_LINE_SENSOR_H

#include "line_sensor_interface.h"
#include "../common/module_test_contract.h"

typedef enum {
    LINE_SENSOR_SURFACE_BLACK = 0,
    LINE_SENSOR_SURFACE_WHITE = 1
} LineSensorSurface;

typedef enum {
    LINE_SENSOR_ERROR_NONE = 0,
    LINE_SENSOR_ERROR_INVALID_INTERFACE = 0x5001U,
    LINE_SENSOR_ERROR_NOT_INITIALIZED = 0x5002U,
    LINE_SENSOR_ERROR_READ_FAILED = 0x5003U,
    LINE_SENSOR_ERROR_NEED_BLACK_SAMPLE = 0x5004U,
    LINE_SENSOR_ERROR_NEED_WHITE_SAMPLE = 0x5005U,
    LINE_SENSOR_ERROR_LOW_CONTRAST = 0x5006U
} LineSensorError;

bool LineSensorTest_Attach(const LineSensorInterface *interface);
void LineSensorTest_Init(uint16_t minimum_contrast);
bool LineSensorTest_CaptureSurface(LineSensorSurface surface);
ModuleTestResult LineSensorTest_RunOnce(void);
void LineSensorTest_SafeStop(void);
bool LineSensorTest_GetBinaryMask(uint8_t *black_mask);
bool LineSensorTest_GetCalibration(uint8_t channel,
                                   uint16_t *black_value,
                                   uint16_t *white_value,
                                   uint16_t *threshold);

#endif
