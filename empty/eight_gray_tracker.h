#ifndef EIGHT_GRAY_TRACKER_H
#define EIGHT_GRAY_TRACKER_H

#include <stdbool.h>
#include <stdint.h>

#define EIGHT_GRAY_SENSOR_COUNT       (8U)
#define EIGHT_GRAY_ACTIVE_FULL_SCALE  (1000U)

typedef enum {
    EIGHT_GRAY_LINE_UNCALIBRATED = 0,
    EIGHT_GRAY_LINE_LOST,
    EIGHT_GRAY_LINE_LEFT,
    EIGHT_GRAY_LINE_CENTER,
    EIGHT_GRAY_LINE_RIGHT,
    EIGHT_GRAY_LINE_INTERSECTION,
} EightGrayLineState;

typedef struct {
    uint16_t white[EIGHT_GRAY_SENSOR_COUNT];
    uint16_t black[EIGHT_GRAY_SENSOR_COUNT];
    uint16_t threshold[EIGHT_GRAY_SENSOR_COUNT];
    uint16_t span[EIGHT_GRAY_SENSOR_COUNT];
    bool darkLow[EIGHT_GRAY_SENSOR_COUNT];
    uint8_t badChannelMask;
    bool valid;
} EightGrayCalibration;

typedef struct {
    uint16_t raw[EIGHT_GRAY_SENSOR_COUNT];
    uint16_t active[EIGHT_GRAY_SENSOR_COUNT];
    uint8_t digitalMask;
    uint8_t activeCount;
    uint32_t activeSum;
    int32_t weightedError;
    EightGrayLineState state;
    bool lineValid;
} EightGrayFrame;

typedef struct {
    uint16_t minimumCalibrationSpan;
    uint16_t digitalActiveThreshold;
    uint16_t minimumLineSum;
    uint8_t intersectionSensorCount;
    int16_t baseCommand;
    int16_t maximumCommand;
    int16_t proportionalGain;
    int16_t derivativeGain;
    int16_t lostSearchFastCommand;
    int16_t lostSearchSlowCommand;
    uint8_t lostSearchFrames;
} EightGrayConfig;

typedef struct {
    int16_t leftCommand;
    int16_t rightCommand;
    bool brake;
    bool searching;
} EightGrayDriveCommand;

typedef struct {
    EightGrayCalibration calibration;
    EightGrayConfig config;
    int32_t previousError;
    int32_t lastVisibleError;
    uint8_t lostFrameCount;
    bool previousErrorValid;
} EightGrayTracker;

void EightGray_init(EightGrayTracker *tracker);
void EightGray_setConfig(
    EightGrayTracker *tracker, const EightGrayConfig *config);
bool EightGray_setCalibration(EightGrayTracker *tracker,
    const uint16_t white[EIGHT_GRAY_SENSOR_COUNT],
    const uint16_t black[EIGHT_GRAY_SENSOR_COUNT]);
void EightGray_analyze(EightGrayTracker *tracker,
    const uint16_t raw[EIGHT_GRAY_SENSOR_COUNT], EightGrayFrame *frame);
void EightGray_resetControl(EightGrayTracker *tracker);
void EightGray_computeDrive(EightGrayTracker *tracker,
    const EightGrayFrame *frame, EightGrayDriveCommand *command);
const char *EightGray_lineStateString(EightGrayLineState state);

#endif
