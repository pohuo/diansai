#include "eight_gray_tracker.h"

#include <string.h>

static const int32_t gEightGrayWeights[EIGHT_GRAY_SENSOR_COUNT] = {
    -3500, -2500, -1500, -500, 500, 1500, 2500, 3500
};

static int16_t eight_gray_clamp_i16(
    int32_t value, int16_t minimum, int16_t maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return (int16_t) value;
}

static uint16_t eight_gray_abs_diff_u16(uint16_t a, uint16_t b)
{
    return (a > b) ? (uint16_t) (a - b) : (uint16_t) (b - a);
}

static uint16_t eight_gray_normalize_active(
    const EightGrayCalibration *calibration, uint8_t index, uint16_t raw)
{
    uint32_t numerator;
    uint16_t span = calibration->span[index];

    if (span == 0U) {
        return 0U;
    }

    if (calibration->darkLow[index]) {
        if (raw >= calibration->white[index]) {
            numerator = 0U;
        } else if (raw <= calibration->black[index]) {
            numerator = span;
        } else {
            numerator = (uint32_t) calibration->white[index] - raw;
        }
    } else {
        if (raw <= calibration->white[index]) {
            numerator = 0U;
        } else if (raw >= calibration->black[index]) {
            numerator = span;
        } else {
            numerator = (uint32_t) raw - calibration->white[index];
        }
    }

    return (uint16_t) ((numerator * EIGHT_GRAY_ACTIVE_FULL_SCALE) / span);
}

static EightGrayLineState eight_gray_classify(
    const EightGrayTracker *tracker, const EightGrayFrame *frame)
{
    if (!tracker->calibration.valid) {
        return EIGHT_GRAY_LINE_UNCALIBRATED;
    }
    if (!frame->lineValid) {
        return EIGHT_GRAY_LINE_LOST;
    }
    if (frame->activeCount >= tracker->config.intersectionSensorCount) {
        return EIGHT_GRAY_LINE_INTERSECTION;
    }
    if (frame->weightedError < -250) {
        return EIGHT_GRAY_LINE_LEFT;
    }
    if (frame->weightedError > 250) {
        return EIGHT_GRAY_LINE_RIGHT;
    }
    return EIGHT_GRAY_LINE_CENTER;
}

void EightGray_init(EightGrayTracker *tracker)
{
    if (tracker == NULL) {
        return;
    }

    memset(tracker, 0, sizeof(*tracker));
    tracker->config.minimumCalibrationSpan = 80U;
    tracker->config.digitalActiveThreshold = 220U;
    tracker->config.minimumLineSum = 500U;
    tracker->config.intersectionSensorCount = 6U;
    tracker->config.baseCommand = 280;
    tracker->config.maximumCommand = 600;
    tracker->config.proportionalGain = 220;
    tracker->config.derivativeGain = 70;
    tracker->config.lostSearchFastCommand = 220;
    tracker->config.lostSearchSlowCommand = 80;
    tracker->config.lostSearchFrames = 15U;
}

void EightGray_setConfig(
    EightGrayTracker *tracker, const EightGrayConfig *config)
{
    if ((tracker == NULL) || (config == NULL)) {
        return;
    }
    tracker->config = *config;
}

bool EightGray_setCalibration(EightGrayTracker *tracker,
    const uint16_t white[EIGHT_GRAY_SENSOR_COUNT],
    const uint16_t black[EIGHT_GRAY_SENSOR_COUNT])
{
    EightGrayCalibration *calibration;

    if ((tracker == NULL) || (white == NULL) || (black == NULL)) {
        return false;
    }

    calibration = &tracker->calibration;
    calibration->badChannelMask = 0U;

    for (uint8_t i = 0U; i < EIGHT_GRAY_SENSOR_COUNT; ++i) {
        calibration->white[i] = white[i];
        calibration->black[i] = black[i];
        calibration->threshold[i] =
            (uint16_t) (((uint32_t) white[i] + black[i]) / 2U);
        calibration->span[i] = eight_gray_abs_diff_u16(white[i], black[i]);
        calibration->darkLow[i] = black[i] < white[i];

        if (calibration->span[i] <
            tracker->config.minimumCalibrationSpan) {
            calibration->badChannelMask |= (uint8_t) (1U << i);
        }
    }

    calibration->valid = calibration->badChannelMask == 0U;
    EightGray_resetControl(tracker);
    return calibration->valid;
}

void EightGray_analyze(EightGrayTracker *tracker,
    const uint16_t raw[EIGHT_GRAY_SENSOR_COUNT], EightGrayFrame *frame)
{
    int32_t weightedSum = 0;

    if ((tracker == NULL) || (raw == NULL) || (frame == NULL)) {
        return;
    }

    memset(frame, 0, sizeof(*frame));
    for (uint8_t i = 0U; i < EIGHT_GRAY_SENSOR_COUNT; ++i) {
        uint16_t active;

        frame->raw[i] = raw[i];
        active = tracker->calibration.valid ?
            eight_gray_normalize_active(&tracker->calibration, i, raw[i]) :
            0U;
        frame->active[i] = active;
        frame->activeSum += active;
        weightedSum += (int32_t) active * gEightGrayWeights[i];

        if (active >= tracker->config.digitalActiveThreshold) {
            frame->digitalMask |= (uint8_t) (1U << i);
            ++frame->activeCount;
        }
    }

    frame->lineValid = tracker->calibration.valid &&
        (frame->activeSum >= tracker->config.minimumLineSum) &&
        (frame->activeCount > 0U);

    if (frame->activeSum > 0U) {
        frame->weightedError = weightedSum / (int32_t) frame->activeSum;
    }

    frame->state = eight_gray_classify(tracker, frame);
    if (frame->lineValid &&
        (frame->state != EIGHT_GRAY_LINE_INTERSECTION)) {
        tracker->lastVisibleError = frame->weightedError;
    }
}

void EightGray_resetControl(EightGrayTracker *tracker)
{
    if (tracker == NULL) {
        return;
    }

    tracker->previousError = 0;
    tracker->lastVisibleError = 0;
    tracker->lostFrameCount = 0U;
    tracker->previousErrorValid = false;
}

void EightGray_computeDrive(EightGrayTracker *tracker,
    const EightGrayFrame *frame, EightGrayDriveCommand *command)
{
    int32_t proportional;
    int32_t derivative = 0;
    int32_t correction;

    if ((tracker == NULL) || (command == NULL)) {
        return;
    }

    memset(command, 0, sizeof(*command));
    command->brake = true;

    if ((frame == NULL) || !tracker->calibration.valid) {
        return;
    }

    if (!frame->lineValid) {
        if ((tracker->lostFrameCount < tracker->config.lostSearchFrames) &&
            (tracker->lastVisibleError != 0)) {
            ++tracker->lostFrameCount;
            command->searching = true;
            command->brake = false;

            if (tracker->lastVisibleError < 0) {
                command->leftCommand =
                    tracker->config.lostSearchSlowCommand;
                command->rightCommand =
                    tracker->config.lostSearchFastCommand;
            } else {
                command->leftCommand =
                    tracker->config.lostSearchFastCommand;
                command->rightCommand =
                    tracker->config.lostSearchSlowCommand;
            }
        }
        tracker->previousErrorValid = false;
        return;
    }

    tracker->lostFrameCount = 0U;
    command->brake = false;

    if (frame->state == EIGHT_GRAY_LINE_INTERSECTION) {
        command->leftCommand = tracker->config.baseCommand;
        command->rightCommand = tracker->config.baseCommand;
        tracker->previousErrorValid = false;
        return;
    }

    proportional =
        (frame->weightedError * tracker->config.proportionalGain) / 3500;
    if (tracker->previousErrorValid) {
        derivative =
            ((frame->weightedError - tracker->previousError) *
                tracker->config.derivativeGain) / 3500;
    }
    tracker->previousError = frame->weightedError;
    tracker->previousErrorValid = true;

    correction = proportional + derivative;
    correction = eight_gray_clamp_i16(correction,
        -tracker->config.maximumCommand, tracker->config.maximumCommand);

    /* Negative error means line is left: slow left, speed right. */
    command->leftCommand = eight_gray_clamp_i16(
        tracker->config.baseCommand + correction,
        0, tracker->config.maximumCommand);
    command->rightCommand = eight_gray_clamp_i16(
        tracker->config.baseCommand - correction,
        0, tracker->config.maximumCommand);
}

const char *EightGray_lineStateString(EightGrayLineState state)
{
    switch (state) {
        case EIGHT_GRAY_LINE_LOST:
            return "LOST";
        case EIGHT_GRAY_LINE_LEFT:
            return "LEFT";
        case EIGHT_GRAY_LINE_CENTER:
            return "CENTER";
        case EIGHT_GRAY_LINE_RIGHT:
            return "RIGHT";
        case EIGHT_GRAY_LINE_INTERSECTION:
            return "INTERSECTION";
        default:
            return "UNCALIBRATED";
    }
}
