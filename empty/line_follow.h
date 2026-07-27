#ifndef LINE_FOLLOW_H
#define LINE_FOLLOW_H

#include "gray_sensor.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t left;
    int16_t right;
    int32_t error;
    bool lineLost;
} LineFollowCommand;

void LineFollow_init(void);
void LineFollow_setBaseSpeed(int16_t speed);
void LineFollow_setSearchSpeed(int16_t speed);
void LineFollow_setGainQ8(uint16_t gainQ8);
void LineFollow_setCenterBias(int32_t bias);
void LineFollow_setWheelTrim(int16_t leftTrim, int16_t rightTrim);
void LineFollow_compute(const GraySensorFrame *frame, LineFollowCommand *command);
int16_t LineFollow_getBaseSpeed(void);
int16_t LineFollow_getSearchSpeed(void);
uint16_t LineFollow_getGainQ8(void);
int32_t LineFollow_getCenterBias(void);
int16_t LineFollow_getLeftTrim(void);
int16_t LineFollow_getRightTrim(void);

#endif
