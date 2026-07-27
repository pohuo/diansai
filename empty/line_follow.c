#include "line_follow.h"

#include <stddef.h>

#define LINE_FOLLOW_MAX_SPEED   (1000)
#define LINE_FOLLOW_MIN_SPEED   (0)
#define LINE_FOLLOW_TRACK_MIN   (100)
#define LINE_FOLLOW_ERROR_SIGN   (1)
#define LINE_FOLLOW_CENTER_BIAS  (500)
#define LINE_FOLLOW_LEFT_TRIM    (0)
#define LINE_FOLLOW_RIGHT_TRIM   (0)

static int16_t gBaseSpeed = 260;
static int16_t gSearchSpeed = 180;
static uint16_t gGainQ8 = 20U;
static int32_t gCenterBias = LINE_FOLLOW_CENTER_BIAS;
static int16_t gLeftTrim = LINE_FOLLOW_LEFT_TRIM;
static int16_t gRightTrim = LINE_FOLLOW_RIGHT_TRIM;
static int32_t gLastError = 0;

static int16_t clamp_speed(int32_t value)
{
    if (value > LINE_FOLLOW_MAX_SPEED) {
        return LINE_FOLLOW_MAX_SPEED;
    }
    if (value < -LINE_FOLLOW_MAX_SPEED) {
        return -LINE_FOLLOW_MAX_SPEED;
    }
    return (int16_t)value;
}

static int16_t clamp_positive_speed(int32_t value)
{
    if (value < LINE_FOLLOW_MIN_SPEED) {
        return LINE_FOLLOW_MIN_SPEED;
    }
    if (value > LINE_FOLLOW_MAX_SPEED) {
        return LINE_FOLLOW_MAX_SPEED;
    }
    return (int16_t)value;
}

void LineFollow_init(void)
{
    gBaseSpeed = 260;
    gSearchSpeed = 180;
    gGainQ8 = 20U;
    gCenterBias = LINE_FOLLOW_CENTER_BIAS;
    gLeftTrim = LINE_FOLLOW_LEFT_TRIM;
    gRightTrim = LINE_FOLLOW_RIGHT_TRIM;
    gLastError = 0;
}

void LineFollow_setBaseSpeed(int16_t speed)
{
    gBaseSpeed = clamp_positive_speed(speed);
}

void LineFollow_setSearchSpeed(int16_t speed)
{
    gSearchSpeed = clamp_positive_speed(speed);
}

void LineFollow_setGainQ8(uint16_t gainQ8)
{
    gGainQ8 = gainQ8;
}

void LineFollow_setCenterBias(int32_t bias)
{
    gCenterBias = bias;
}

void LineFollow_setWheelTrim(int16_t leftTrim, int16_t rightTrim)
{
    gLeftTrim = leftTrim;
    gRightTrim = rightTrim;
}

void LineFollow_compute(const GraySensorFrame *frame, LineFollowCommand *command)
{
    int32_t correction;
    int32_t error;
    int32_t left;
    int32_t right;

    if ((frame == NULL) || (command == NULL)) {
        return;
    }

    command->lineLost = (frame->mask == 0U);

    if (command->lineLost) {
        if (gLastError >= 0) {
            command->left = gSearchSpeed;
            command->right = (int16_t)-gSearchSpeed;
        } else {
            command->left = (int16_t)-gSearchSpeed;
            command->right = gSearchSpeed;
        }
        return;
    }

    error = frame->error - gCenterBias;
    command->error = error;
    gLastError = error;

    correction = ((int32_t)LINE_FOLLOW_ERROR_SIGN * error *
        (int32_t)gGainQ8) >> 8;
    left = (int32_t)gBaseSpeed + correction + (int32_t)gLeftTrim;
    right = (int32_t)gBaseSpeed - correction + (int32_t)gRightTrim;

    if (left < LINE_FOLLOW_TRACK_MIN) {
        left = LINE_FOLLOW_TRACK_MIN;
    }
    if (right < LINE_FOLLOW_TRACK_MIN) {
        right = LINE_FOLLOW_TRACK_MIN;
    }

    command->left = clamp_speed(left);
    command->right = clamp_speed(right);
}

int16_t LineFollow_getBaseSpeed(void)
{
    return gBaseSpeed;
}

int16_t LineFollow_getSearchSpeed(void)
{
    return gSearchSpeed;
}

uint16_t LineFollow_getGainQ8(void)
{
    return gGainQ8;
}

int32_t LineFollow_getCenterBias(void)
{
    return gCenterBias;
}

int16_t LineFollow_getLeftTrim(void)
{
    return gLeftTrim;
}

int16_t LineFollow_getRightTrim(void)
{
    return gRightTrim;
}
