#ifndef LINE_SENSOR_INTERFACE_H
#define LINE_SENSOR_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LINE_SENSOR_CHANNEL_COUNT (4U)

/*
 * Channels are ordered from the vehicle's left to right: 0, 1, 2, 3.
 * Digital adapters return 0 or 1. Analog adapters return raw ADC values.
 */
typedef struct {
    bool (*read_channel)(uint8_t channel, uint16_t *raw_value);
} LineSensorInterface;

#endif
