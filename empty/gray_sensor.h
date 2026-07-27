#ifndef GRAY_SENSOR_H
#define GRAY_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

#define GRAY_SENSOR_CHANNELS 8U

typedef struct {
    uint16_t raw[GRAY_SENSOR_CHANNELS];
    uint16_t threshold[GRAY_SENSOR_CHANNELS];
    uint8_t mask;
    int32_t error;
} GraySensorFrame;

void GraySensor_init(void);
void GraySensor_resetCalibration(void);
bool GraySensor_captureWhite(void);
bool GraySensor_captureBlack(void);
void GraySensor_read(GraySensorFrame *frame);
const uint16_t *GraySensor_getThresholds(void);

#endif
