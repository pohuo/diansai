#include "test_vision_uart.h"

#include <limits.h>

static VisionUartInterface gInterface;
static VisionUartTarget gTarget;
static VisionUartStats gStats;
static VisionUartError gLastError;
static char gFrame[VISION_UART_FRAME_MAX_LENGTH];
static uint8_t gFrameLength;
static uint8_t gLastSequence;
static uint32_t gLastFrameAtMs;
static bool gAttached;
static bool gInitialized;
static bool gCollecting;
static bool gHaveFrame;
static bool gHaveSequence;
static bool gTimeoutLatched;

static bool VisionParseUnsigned(const char **cursor,
                                char terminator,
                                uint32_t maximum,
                                uint32_t *value)
{
    const char *position = *cursor;
    uint32_t parsed = 0U;
    bool hasDigit = false;

    while ((*position >= '0') && (*position <= '9')) {
        uint32_t digit = (uint32_t) (*position - '0');
        hasDigit = true;
        if ((digit > maximum) ||
            (parsed > ((maximum - digit) / 10U))) {
            return false;
        }
        parsed = (parsed * 10U) + digit;
        position++;
    }

    if (!hasDigit || (*position != terminator)) {
        return false;
    }

    *cursor = (terminator == '\0') ? position : position + 1;
    *value = parsed;
    return true;
}

static void VisionRecordFrameError(VisionUartError error)
{
    gLastError = error;
    gStats.consecutive_good_frames = 0U;

    if (error == VISION_UART_ERROR_CHECKSUM) {
        gStats.checksum_errors++;
    } else if (error == VISION_UART_ERROR_RANGE) {
        gStats.range_errors++;
    } else {
        gStats.format_errors++;
    }
}

static bool VisionParseFrame(uint32_t nowMs)
{
    uint8_t checksumComma = 0U;
    uint8_t index;
    uint32_t checksum = 0U;
    uint32_t receivedChecksum;
    uint32_t sequence;
    uint32_t valid;
    uint32_t x;
    uint32_t y;
    const char *cursor;
    uint8_t expectedSequence;

    if ((gFrameLength < 12U) ||
        (gFrame[gFrameLength - 1U] != '*')) {
        VisionRecordFrameError(VISION_UART_ERROR_FORMAT);
        return false;
    }

    gFrame[gFrameLength - 1U] = '\0';
    for (index = 0U; index < (gFrameLength - 1U); index++) {
        if (gFrame[index] == ',') {
            checksumComma = index;
        }
    }

    if ((checksumComma == 0U) || (gFrame[0] != 'V') ||
        (gFrame[1] != ',')) {
        VisionRecordFrameError(VISION_UART_ERROR_FORMAT);
        return false;
    }

    cursor = &gFrame[checksumComma + 1U];
    if (!VisionParseUnsigned(&cursor, '\0', 254U, &receivedChecksum)) {
        VisionRecordFrameError(VISION_UART_ERROR_FORMAT);
        return false;
    }

    for (index = 0U; index < checksumComma; index++) {
        checksum += (uint8_t) gFrame[index];
    }
    checksum %= 255U;
    if (checksum != receivedChecksum) {
        VisionRecordFrameError(VISION_UART_ERROR_CHECKSUM);
        return false;
    }

    gFrame[checksumComma] = '\0';
    cursor = &gFrame[2];
    if (!VisionParseUnsigned(&cursor, ',', 255U, &sequence) ||
        !VisionParseUnsigned(&cursor, ',', 1U, &valid) ||
        !VisionParseUnsigned(&cursor, ',', VISION_UART_IMAGE_WIDTH - 1U, &x) ||
        !VisionParseUnsigned(&cursor, '\0', VISION_UART_IMAGE_HEIGHT - 1U, &y)) {
        VisionRecordFrameError(VISION_UART_ERROR_RANGE);
        return false;
    }

    if ((valid == 0U) && ((x != 0U) || (y != 0U))) {
        VisionRecordFrameError(VISION_UART_ERROR_RANGE);
        return false;
    }

    if (gHaveSequence) {
        expectedSequence = (uint8_t) (gLastSequence + 1U);
        if ((uint8_t) sequence != expectedSequence) {
            gStats.sequence_errors++;
            gStats.consecutive_good_frames = 0U;
        }
    }

    gLastSequence = (uint8_t) sequence;
    gHaveSequence = true;
    gTarget.sequence = (uint8_t) sequence;
    gTarget.valid = (valid != 0U);
    gTarget.x = (uint16_t) x;
    gTarget.y = (uint16_t) y;
    gTarget.received_at_ms = nowMs;
    gLastFrameAtMs = nowMs;
    gHaveFrame = true;
    gTimeoutLatched = false;
    gLastError = VISION_UART_ERROR_NONE;
    gStats.accepted_frames++;
    gStats.consecutive_good_frames++;
    return true;
}

static void VisionProcessByte(uint8_t byte, uint32_t nowMs)
{
    if (byte == '$') {
        gCollecting = true;
        gFrameLength = 0U;
        return;
    }

    if (!gCollecting || (byte == '\r')) {
        return;
    }

    if (byte == '\n') {
        (void) VisionParseFrame(nowMs);
        gCollecting = false;
        gFrameLength = 0U;
        return;
    }

    if (gFrameLength >= (VISION_UART_FRAME_MAX_LENGTH - 1U)) {
        VisionRecordFrameError(VISION_UART_ERROR_FORMAT);
        gCollecting = false;
        gFrameLength = 0U;
        return;
    }

    gFrame[gFrameLength] = (char) byte;
    gFrameLength++;
}

bool VisionUartTest_Attach(const VisionUartInterface *interface)
{
    if ((interface == NULL) || (interface->get_time_ms == NULL) ||
        (interface->read_byte == NULL) ||
        (interface->handle_link_timeout == NULL)) {
        gAttached = false;
        return false;
    }

    gInterface = *interface;
    gAttached = true;
    return true;
}

void VisionUartTest_Init(void)
{
    gTarget.sequence = 0U;
    gTarget.valid = false;
    gTarget.x = 0U;
    gTarget.y = 0U;
    gTarget.received_at_ms = 0U;
    gStats = (VisionUartStats) {0};
    gLastError = VISION_UART_ERROR_NONE;
    gFrameLength = 0U;
    gLastSequence = 0U;
    gCollecting = false;
    gHaveFrame = false;
    gHaveSequence = false;
    gTimeoutLatched = false;
    gInitialized = gAttached;

    if (gInitialized) {
        gLastFrameAtMs = gInterface.get_time_ms();
    }
}

ModuleTestResult VisionUartTest_RunOnce(void)
{
    ModuleTestResult result = {
        MODULE_TEST_BLOCKED,
        0,
        (int32_t) VISION_UART_REQUIRED_FRAMES,
        VISION_UART_ERROR_NOT_INITIALIZED
    };
    uint8_t byte;
    uint8_t processed = 0U;
    uint32_t nowMs;

    if (!gInitialized) {
        result.error_code = gAttached ? VISION_UART_ERROR_NOT_INITIALIZED
                                      : VISION_UART_ERROR_INVALID_INTERFACE;
        return result;
    }

    nowMs = gInterface.get_time_ms();
    while ((processed < VISION_UART_MAX_BYTES_PER_RUN) &&
           gInterface.read_byte(&byte)) {
        VisionProcessByte(byte, nowMs);
        processed++;
    }

    nowMs = gInterface.get_time_ms();
    if (!gTimeoutLatched &&
        ((uint32_t) (nowMs - gLastFrameAtMs) > VISION_UART_TIMEOUT_MS)) {
        gTarget.valid = false;
        gTarget.x = 0U;
        gTarget.y = 0U;
        gStats.consecutive_good_frames = 0U;
        gStats.timeout_events++;
        gLastError = VISION_UART_ERROR_TIMEOUT;
        gTimeoutLatched = true;
        gInterface.handle_link_timeout();
    }

    result.measured_value =
        (gStats.consecutive_good_frames > (uint32_t) INT32_MAX)
            ? INT32_MAX
            : (int32_t) gStats.consecutive_good_frames;
    result.error_code = (uint32_t) gLastError;

    if (gTimeoutLatched) {
        result.status = MODULE_TEST_FAILED;
    } else if (gStats.consecutive_good_frames >=
               VISION_UART_REQUIRED_FRAMES) {
        result.status = MODULE_TEST_PASSED;
    } else {
        result.status = MODULE_TEST_RUNNING;
    }

    return result;
}

void VisionUartTest_SafeStop(void)
{
    if (gAttached) {
        gInterface.handle_link_timeout();
    }
    gTarget.valid = false;
    gInitialized = false;
}

bool VisionUartTest_GetLatestTarget(VisionUartTarget *target)
{
    if ((target == NULL) || !gHaveFrame || gTimeoutLatched) {
        return false;
    }

    *target = gTarget;
    return true;
}

void VisionUartTest_GetStats(VisionUartStats *stats)
{
    if (stats != NULL) {
        *stats = gStats;
    }
}
