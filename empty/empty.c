/*
 * MSPM0G3507 eight-channel gray sensor + DRV8701 drive firmware
 *
 * Safety defaults:
 *   - Read and calibrate 8-channel gray values through 74HC4051
 *   - Keep both motors braked after reset
 *   - Require an explicit serial ARM or AUTO command before motion
 *   - Brake immediately when automatic tracking loses the line
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ti_msp_dl_config.h"
#include "drv8701.h"

#define GRAY_SENSOR_COUNT          (8U)
#define GRAY_SAMPLE_AVG            (4U)
#define GRAY_SETTLE_US             (8U)
#define GRAY_CONTROL_INTERVAL_MS    (10U)
#define GRAY_REPORT_INTERVAL_MS    (200U)
#define GRAY_HEARTBEAT_INTERVAL_MS (1000U)
#define GRAY_CAL_AVG_FRAMES        (8U)
#define GRAY_DEFAULT_THRESHOLD     (1500U)
#define GRAY_ACTIVE_MIN_THRESHOLD   (50U)
#define GRAY_LINE_SUM_THRESHOLD    (180U)

#define DRIVE_AUTO_BASE_COMMAND    (280)
#define DRIVE_AUTO_MAX_COMMAND     (600)
#define DRIVE_AUTO_CORRECTION_MAX  (240)
#define DRIVE_MANUAL_TIMEOUT_MS    (1000U)

#define UART_CMD_BUF_LEN           (32U)

typedef enum {
    STREAM_RAW = 0,
    STREAM_CALIBRATED = 1,
    STREAM_STOPPED = 2,
} StreamMode;

typedef enum {
    TURN_LOST = 0,
    TURN_LEFT = 1,
    TURN_STRAIGHT = 2,
    TURN_RIGHT = 3,
} TurnHint;

typedef enum {
    DRIVE_DISARMED = 0,
    DRIVE_MANUAL = 1,
    DRIVE_AUTO = 2,
} DriveMode;

typedef struct {
    uint16_t raw[GRAY_SENSOR_COUNT];
    uint16_t active[GRAY_SENSOR_COUNT];
    uint16_t white[GRAY_SENSOR_COUNT];
    uint16_t black[GRAY_SENSOR_COUNT];
    uint16_t threshold[GRAY_SENSOR_COUNT];
    uint16_t span[GRAY_SENSOR_COUNT];
    bool darkLow[GRAY_SENSOR_COUNT];
    bool whiteValid;
    bool blackValid;
    bool calibrated;
    bool lineValid;
    uint8_t digitalMask;
    uint32_t activeSum;
    int32_t weightedError;
    TurnHint turnHint;
} GrayCalib;

static volatile bool gAdcReady = false;
static GrayCalib gCal = {0};
static StreamMode gStreamMode = STREAM_RAW;
static char gCmdBuf[UART_CMD_BUF_LEN];
static uint8_t gCmdLen = 0U;
static DriveMode gDriveMode = DRIVE_DISARMED;
static uint32_t gManualAgeMs = 0U;

static const int32_t gGrayWeights[GRAY_SENSOR_COUNT] = {
    -3500, -2500, -1500, -500, 500, 1500, 2500, 3500
};

static uint16_t abs_diff_u16(uint16_t a, uint16_t b)
{
    return (a > b) ? (uint16_t)(a - b) : (uint16_t)(b - a);
}

static const char *gray_turn_hint_string(TurnHint hint)
{
    switch (hint) {
        case TURN_LEFT:
            return "LEFT";
        case TURN_STRAIGHT:
            return "STRAIGHT";
        case TURN_RIGHT:
            return "RIGHT";
        default:
            return "LOST";
    }
}

static uint16_t gray_min_u16(const uint16_t *data)
{
    uint16_t minv = data[0];

    for (uint8_t i = 1U; i < GRAY_SENSOR_COUNT; ++i) {
        if (data[i] < minv) {
            minv = data[i];
        }
    }

    return minv;
}

static uint16_t gray_max_u16(const uint16_t *data)
{
    uint16_t maxv = data[0];

    for (uint8_t i = 1U; i < GRAY_SENSOR_COUNT; ++i) {
        if (data[i] > maxv) {
            maxv = data[i];
        }
    }

    return maxv;
}

static uint8_t gray_sat_count(const uint16_t *data)
{
    uint8_t sat = 0U;

    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; ++i) {
        if ((data[i] == 0U) || (data[i] >= 4095U)) {
            ++sat;
        }
    }

    return sat;
}

static char ascii_upper(char ch)
{
    if (ch >= 'a' && ch <= 'z') {
        return (char)(ch - ('a' - 'A'));
    }
    return ch;
}

static bool command_equals(const char *cmd, const char *ref)
{
    while (*cmd == ' ' || *cmd == '\t') {
        ++cmd;
    }

    while (*cmd != '\0' && *ref != '\0') {
        if (ascii_upper(*cmd) != ascii_upper(*ref)) {
            return false;
        }
        ++cmd;
        ++ref;
    }

    while (*cmd == ' ' || *cmd == '\t') {
        ++cmd;
    }

    return (*cmd == '\0') && (*ref == '\0');
}

static void delay_us(uint32_t us)
{
    delay_cycles((CPUCLK_FREQ / 1000000U) * us);
}

static void delay_ms(uint32_t ms)
{
    delay_cycles((CPUCLK_FREQ / 1000U) * ms);
}

static void uart_write_char(char ch)
{
    DL_UART_transmitDataBlocking(UART_0_INST, (uint8_t)ch);
}

static void uart_write_string(const char *str)
{
    if (str == NULL) {
        return;
    }

    while (*str != '\0') {
        uart_write_char(*str++);
    }
}

static void uart_write_crlf(void)
{
    uart_write_string("\r\n");
}

static void uart_write_u32(uint32_t value)
{
    char buf[16];
    (void)snprintf(buf, sizeof(buf), "%lu", (unsigned long)value);
    uart_write_string(buf);
}

static void uart_write_u16(uint16_t value)
{
    uart_write_u32((uint32_t)value);
}

static void uart_write_i32(int32_t value)
{
    char buf[16];
    (void)snprintf(buf, sizeof(buf), "%ld", (long)value);
    uart_write_string(buf);
}

static void uart_write_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    uart_write_char(hex[(value >> 4) & 0x0FU]);
    uart_write_char(hex[value & 0x0FU]);
}

static void uart_write_array_u16(const char *label, const uint16_t *data,
    uint8_t count)
{
    if ((label != NULL) && (label[0] != '\0')) {
        uart_write_string(label);
        uart_write_char(',');
    }

    for (uint8_t i = 0U; i < count; ++i) {
        uart_write_u16(data[i]);
        if ((uint8_t)(i + 1U) < count) {
            uart_write_char(',');
        }
    }
}

/* ----------------------------- Gray sensor scan -------------------------- */

static void gray_set_address(uint8_t index)
{
    if ((index & 0x01U) != 0U) {
        DL_GPIO_setPins(GRAY_ADDR_A0_PORT, GRAY_ADDR_A0_PIN);
    } else {
        DL_GPIO_clearPins(GRAY_ADDR_A0_PORT, GRAY_ADDR_A0_PIN);
    }

    if ((index & 0x02U) != 0U) {
        DL_GPIO_setPins(GRAY_ADDR_A1_PORT, GRAY_ADDR_A1_PIN);
    } else {
        DL_GPIO_clearPins(GRAY_ADDR_A1_PORT, GRAY_ADDR_A1_PIN);
    }

    if ((index & 0x04U) != 0U) {
        DL_GPIO_setPins(GRAY_ADDR_A2_PORT, GRAY_ADDR_A2_PIN);
    } else {
        DL_GPIO_clearPins(GRAY_ADDR_A2_PORT, GRAY_ADDR_A2_PIN);
    }
}

static uint16_t gray_sample_once(void)
{
    gAdcReady = false;
    DL_ADC12_clearInterruptStatus(ADC12_0_INST,
        DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(ADC12_0_INST);

    while (!gAdcReady) {
        __WFE();
    }

    return DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_ADC_CH0);
}

static uint16_t gray_read_channel(uint8_t index)
{
    uint32_t sum = 0U;

    gray_set_address(index);
    delay_us(GRAY_SETTLE_US);

    for (uint8_t i = 0U; i < GRAY_SAMPLE_AVG; ++i) {
        sum += gray_sample_once();
    }

    return (uint16_t)(sum / GRAY_SAMPLE_AVG);
}

static void gray_read_frame(GrayCalib *frame)
{
    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; ++i) {
        frame->raw[i] = gray_read_channel(i);
    }
}

static void gray_capture_reference(uint16_t dest[GRAY_SENSOR_COUNT])
{
    uint32_t accum[GRAY_SENSOR_COUNT] = {0};
    GrayCalib frame = {0};

    for (uint8_t n = 0U; n < GRAY_CAL_AVG_FRAMES; ++n) {
        gray_read_frame(&frame);
        for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; ++i) {
            accum[i] += frame.raw[i];
        }
        delay_ms(8U);
    }

    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; ++i) {
        dest[i] = (uint16_t)(accum[i] / GRAY_CAL_AVG_FRAMES);
    }
}

static void gray_print_reference_summary(const char *tag,
    const uint16_t ref[GRAY_SENSOR_COUNT])
{
    uint32_t sum = 0U;

    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; ++i) {
        sum += ref[i];
    }

    uart_write_string(tag);
    uart_write_string(",MIN,");
    uart_write_u16(gray_min_u16(ref));
    uart_write_string(",MAX,");
    uart_write_u16(gray_max_u16(ref));
    uart_write_string(",AVG,");
    uart_write_u16((uint16_t)(sum / GRAY_SENSOR_COUNT));
    uart_write_string(",SAT,");
    uart_write_u32(gray_sat_count(ref));
    uart_write_string(",DATA,");
    uart_write_array_u16("", ref, GRAY_SENSOR_COUNT);
    uart_write_crlf();
}

static void gray_update_calibration(void)
{
    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; ++i) {
        gCal.threshold[i] = (uint16_t)((gCal.white[i] + gCal.black[i]) / 2U);
        gCal.span[i] = abs_diff_u16(gCal.white[i], gCal.black[i]);
        gCal.darkLow[i] = (gCal.black[i] < gCal.white[i]);
    }

    gCal.calibrated = gCal.whiteValid && gCal.blackValid;

    if (gCal.calibrated) {
        gStreamMode = STREAM_CALIBRATED;
    }
}

static void gray_capture_white(void)
{
    gray_capture_reference(gCal.white);
    gCal.whiteValid = true;
    gray_update_calibration();
    gray_print_reference_summary("WHITE", gCal.white);
}

static void gray_capture_black(void)
{
    gray_capture_reference(gCal.black);
    gCal.blackValid = true;
    gray_update_calibration();
    gray_print_reference_summary("BLACK", gCal.black);
}

static uint16_t gray_active_from_raw(uint8_t idx, uint16_t raw)
{
    uint16_t threshold = GRAY_DEFAULT_THRESHOLD;
    bool darkLow = true;

    if (gCal.calibrated) {
        threshold = gCal.threshold[idx];
        darkLow = gCal.darkLow[idx];
    }

    if (darkLow) {
        return (raw < threshold) ? (uint16_t)(threshold - raw) : 0U;
    }

    return (raw > threshold) ? (uint16_t)(raw - threshold) : 0U;
}

static int32_t gray_analyze_frame(GrayCalib *frame)
{
    int32_t weightedSum = 0;
    uint32_t activeSum = 0U;
    uint8_t mask = 0U;

    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; ++i) {
        uint16_t active = gray_active_from_raw(i, frame->raw[i]);
        uint16_t activeThreshold = GRAY_ACTIVE_MIN_THRESHOLD;

        if (gCal.calibrated && (gCal.span[i] > 0U)) {
            uint16_t spanThreshold = (uint16_t)(gCal.span[i] / 8U);
            if (spanThreshold > activeThreshold) {
                activeThreshold = spanThreshold;
            }
        }

        frame->active[i] = active;
        activeSum += active;
        weightedSum += (int32_t)active * gGrayWeights[i];

        if (active > activeThreshold) {
            mask |= (uint8_t)(1U << i);
        }
    }

    frame->digitalMask = mask;
    frame->activeSum = activeSum;
    frame->lineValid = (activeSum >= GRAY_LINE_SUM_THRESHOLD);

    if (activeSum == 0U) {
        frame->weightedError = 0;
        frame->turnHint = TURN_LOST;
        return 0;
    }

    frame->weightedError = weightedSum / (int32_t)activeSum;
    if (!frame->lineValid) {
        frame->turnHint = TURN_STRAIGHT;
    } else if (frame->weightedError < -250) {
        frame->turnHint = TURN_LEFT;
    } else if (frame->weightedError > 250) {
        frame->turnHint = TURN_RIGHT;
    } else {
        frame->turnHint = TURN_STRAIGHT;
    }

    return frame->weightedError;
}

/* ---------------------------- DRV8701 control ---------------------------- */

static const char *drive_mode_string(DriveMode mode)
{
    switch (mode) {
        case DRIVE_MANUAL:
            return "MANUAL";
        case DRIVE_AUTO:
            return "AUTO";
        default:
            return "DISARMED";
    }
}

static int16_t drive_clamp(int32_t value, int16_t minimum, int16_t maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return (int16_t) value;
}

static void drive_emergency_stop(void)
{
    DRV8701_brake();
    gDriveMode = DRIVE_DISARMED;
    gManualAgeMs = 0U;
}

static void drive_arm_manual(void)
{
    DRV8701_brake();
    gDriveMode = DRIVE_MANUAL;
    gManualAgeMs = DRIVE_MANUAL_TIMEOUT_MS;
}

static bool drive_start_auto(void)
{
    DRV8701_brake();
    if (!gCal.calibrated) {
        gDriveMode = DRIVE_DISARMED;
        return false;
    }

    gDriveMode = DRIVE_AUTO;
    return true;
}

static bool drive_parse_manual_command(
    const char *cmd, int16_t *left, int16_t *right)
{
    char normalized[UART_CMD_BUF_LEN];
    long leftValue = 0;
    long rightValue = 0;
    size_t length;

    if ((cmd == NULL) || (left == NULL) || (right == NULL) ||
        (ascii_upper(cmd[0]) != 'M')) {
        return false;
    }

    length = strlen(cmd);
    if (length >= sizeof(normalized)) {
        return false;
    }

    memcpy(normalized, cmd, length + 1U);
    for (size_t i = 0U; i < length; ++i) {
        if (normalized[i] == ',') {
            normalized[i] = ' ';
        }
    }

    if (sscanf(&normalized[1], "%ld %ld", &leftValue, &rightValue) != 2) {
        return false;
    }

    *left = drive_clamp(leftValue,
        -DRV8701_COMMAND_MAX, DRV8701_COMMAND_MAX);
    *right = drive_clamp(rightValue,
        -DRV8701_COMMAND_MAX, DRV8701_COMMAND_MAX);
    return true;
}

static void drive_update(const GrayCalib *frame)
{
    int32_t correction;
    int16_t left;
    int16_t right;

    if (gDriveMode == DRIVE_DISARMED) {
        DRV8701_brake();
        return;
    }

    if (gDriveMode == DRIVE_MANUAL) {
        if (gManualAgeMs < DRIVE_MANUAL_TIMEOUT_MS) {
            gManualAgeMs += GRAY_CONTROL_INTERVAL_MS;
        }
        if (gManualAgeMs >= DRIVE_MANUAL_TIMEOUT_MS) {
            DRV8701_brake();
        }
        return;
    }

    if ((frame == NULL) || !gCal.calibrated || !frame->lineValid) {
        DRV8701_brake();
        return;
    }

    correction = (frame->weightedError * DRIVE_AUTO_CORRECTION_MAX) / 3500;
    correction = drive_clamp(correction,
        -DRIVE_AUTO_CORRECTION_MAX, DRIVE_AUTO_CORRECTION_MAX);

    /* Negative error means line is left: slow left wheel, speed right wheel. */
    left = drive_clamp(DRIVE_AUTO_BASE_COMMAND + correction,
        0, DRIVE_AUTO_MAX_COMMAND);
    right = drive_clamp(DRIVE_AUTO_BASE_COMMAND - correction,
        0, DRIVE_AUTO_MAX_COMMAND);
    DRV8701_setMotors(left, right);
}
/* ------------------------------ UART helpers ----------------------------- */

static void gray_print_help(void)
{
    uart_write_string("Sensor: H=help, R=raw, C=calibrated, ");
    uart_write_string("S=stream stop, W=white, B=black, T=status");
    uart_write_crlf();
    uart_write_string("Drive: ARM, M <left> <right>, AUTO/GO, ");
    uart_write_string("X/STOP/ESTOP");
    uart_write_crlf();
    uart_write_string("Drive range: -1000..1000; manual watchdog: 1000 ms");
    uart_write_crlf();
}

static void gray_print_status(void)
{
    uart_write_string("STATUS,CAL,");
    uart_write_u32(gCal.calibrated ? 1U : 0U);
    uart_write_string(",W_OK,");
    uart_write_u32(gCal.whiteValid ? 1U : 0U);
    uart_write_string(",B_OK,");
    uart_write_u32(gCal.blackValid ? 1U : 0U);
    uart_write_string(",WHITE,");
    uart_write_array_u16("", gCal.white, GRAY_SENSOR_COUNT);
    uart_write_string(",BLACK,");
    uart_write_array_u16("", gCal.black, GRAY_SENSOR_COUNT);
    uart_write_string(",THR,");
    uart_write_array_u16("", gCal.threshold, GRAY_SENSOR_COUNT);
    uart_write_string(",SPAN,");
    uart_write_array_u16("", gCal.span, GRAY_SENSOR_COUNT);
    uart_write_string(",MASK,0x");
    uart_write_hex8(gCal.digitalMask);
    uart_write_string(",TURN,");
    uart_write_string(gray_turn_hint_string(gCal.turnHint));
    uart_write_string(",DRIVE,");
    uart_write_string(drive_mode_string(gDriveMode));
    uart_write_string(",LEFT,");
    uart_write_i32(DRV8701_getLeftCommand());
    uart_write_string(",RIGHT,");
    uart_write_i32(DRV8701_getRightCommand());
    uart_write_crlf();
}

static void gray_print_frame(const GrayCalib *frame)
{
    uart_write_string("RAW,");
    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; ++i) {
        uart_write_u16(frame->raw[i]);
        if ((uint8_t)(i + 1U) < GRAY_SENSOR_COUNT) {
            uart_write_char(',');
        }
    }

    uart_write_string(",ACT,");
    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; ++i) {
        uart_write_u16(frame->active[i]);
        if ((uint8_t)(i + 1U) < GRAY_SENSOR_COUNT) {
            uart_write_char(',');
        }
    }

    uart_write_string(",ERR,");
    uart_write_i32(frame->weightedError);
    uart_write_string(",SUM,");
    uart_write_u32(frame->activeSum);
    uart_write_string(",LINE,");
    uart_write_u32(frame->lineValid ? 1U : 0U);
    uart_write_string(",MASK,0x");
    uart_write_hex8(frame->digitalMask);
    uart_write_string(",TURN,");
    uart_write_string(gray_turn_hint_string(frame->turnHint));
    uart_write_string(",CAL,");
    uart_write_u32(gCal.calibrated ? 1U : 0U);
    uart_write_crlf();
}

static void gray_handle_command(const char *cmd)
{
    int16_t leftCommand = 0;
    int16_t rightCommand = 0;

    if (cmd == NULL || *cmd == '\0') {
        return;
    }

    if (command_equals(cmd, "H") || command_equals(cmd, "HELP") ||
        command_equals(cmd, "?")) {
        gray_print_help();
        return;
    }

    if (command_equals(cmd, "X") || command_equals(cmd, "STOP") ||
        command_equals(cmd, "ESTOP")) {
        drive_emergency_stop();
        uart_write_string("DRIVE,DISARMED,BRAKE");
        uart_write_crlf();
        return;
    }

    if (command_equals(cmd, "ARM")) {
        drive_arm_manual();
        uart_write_string("DRIVE,MANUAL,ARMED");
        uart_write_crlf();
        return;
    }

    if (command_equals(cmd, "AUTO") || command_equals(cmd, "GO")) {
        if (drive_start_auto()) {
            uart_write_string("DRIVE,AUTO,ARMED");
        } else {
            uart_write_string("ERR,AUTO_REQUIRES_CALIBRATION");
        }
        uart_write_crlf();
        return;
    }

    if (drive_parse_manual_command(
            cmd, &leftCommand, &rightCommand)) {
        if (gDriveMode != DRIVE_MANUAL) {
            uart_write_string("ERR,DRIVE_NOT_ARMED,SEND_ARM_FIRST");
        } else {
            DRV8701_setMotors(leftCommand, rightCommand);
            gManualAgeMs = 0U;
            uart_write_string("DRIVE,MANUAL,LEFT,");
            uart_write_i32(leftCommand);
            uart_write_string(",RIGHT,");
            uart_write_i32(rightCommand);
        }
        uart_write_crlf();
        return;
    }

    if (ascii_upper(cmd[0]) == 'M') {
        uart_write_string("ERR,MANUAL_FORMAT,M <left> <right>");
        uart_write_crlf();
        return;
    }

    if (command_equals(cmd, "R") || command_equals(cmd, "RAW")) {
        gStreamMode = STREAM_RAW;
        uart_write_string("MODE,RAW");
        uart_write_crlf();
        return;
    }

    if (command_equals(cmd, "C") || command_equals(cmd, "CAL")) {
        gStreamMode = STREAM_CALIBRATED;
        uart_write_string("MODE,CAL");
        uart_write_crlf();
        return;
    }

    if (command_equals(cmd, "S") || command_equals(cmd, "STREAMSTOP")) {
        gStreamMode = STREAM_STOPPED;
        uart_write_string("MODE,STREAM_STOPPED");
        uart_write_crlf();
        return;
    }

    if (command_equals(cmd, "W") || command_equals(cmd, "WHITE")) {
        drive_emergency_stop();
        gray_capture_white();
        uart_write_string("CAPTURE,WHITE");
        uart_write_crlf();
        gray_print_status();
        if (gCal.calibrated) {
            uart_write_string("CAL,READY");
            uart_write_crlf();
        }
        return;
    }

    if (command_equals(cmd, "B") || command_equals(cmd, "BLACK")) {
        drive_emergency_stop();
        gray_capture_black();
        uart_write_string("CAPTURE,BLACK");
        uart_write_crlf();
        gray_print_status();
        if (gCal.calibrated) {
            uart_write_string("CAL,READY");
            uart_write_crlf();
        }
        return;
    }

    if (command_equals(cmd, "T") || command_equals(cmd, "STATUS")) {
        gray_print_status();
        return;
    }

    uart_write_string("ERR,UNKNOWN_CMD");
    uart_write_crlf();
}
static void uart_poll_commands(void)
{
    while (!DL_UART_isRXFIFOEmpty(UART_0_INST)) {
        char ch = (char)DL_UART_receiveData(UART_0_INST);

        uart_write_string("RX,0x");
        uart_write_hex8((uint8_t)ch);
        uart_write_crlf();

        /* X is the only immediate command; all others terminate with CR/LF. */
        if ((gCmdLen == 0U) && (ascii_upper(ch) == 'X')) {
            gray_handle_command("X");
            continue;
        }

        if (ch == '\r' || ch == '\n') {
            if (gCmdLen > 0U) {
                gCmdBuf[gCmdLen] = '\0';
                gray_handle_command(gCmdBuf);
                gCmdLen = 0U;
            }
            continue;
        }

        if (gCmdLen < (UART_CMD_BUF_LEN - 1U)) {
            gCmdBuf[gCmdLen++] = ch;
        } else {
            gCmdLen = 0U;
        }
    }
}

/* ------------------------------ ADC ISR hook ----------------------------- */

void ADC12_0_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC12_0_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            gAdcReady = true;
            DL_ADC12_clearInterruptStatus(ADC12_0_INST,
                DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
            break;
        default:
            break;
    }
}

/* --------------------------------- main ---------------------------------- */

int main(void)
{
    GrayCalib frame = {0};
    uint32_t reportElapsedMs = 0U;
    uint32_t heartbeatElapsedMs = 0U;

    SYSCFG_DL_init();
    DRV8701_init();
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);

    uart_write_string("\r\nBOOT,GRAY_DRV8701\r\n");
    gray_print_help();
    uart_write_string("NOTE,HC05,9600,8N1");
    uart_write_crlf();
    uart_write_string("NOTE,DRV8701,20KHZ,POWER_ON_DISARMED");
    uart_write_crlf();
    uart_write_string("PIN,LEFT_EN,PB2,LEFT_PH,PA14");
    uart_write_crlf();
    uart_write_string("PIN,RIGHT_EN,PB3,RIGHT_PH,PA16");
    uart_write_crlf();
    uart_write_string("STREAM,RAW");
    uart_write_crlf();

    while (1) {
        bool frameNeeded;

        uart_poll_commands();
        frameNeeded = (gStreamMode != STREAM_STOPPED) ||
            (gDriveMode == DRIVE_AUTO);

        if (frameNeeded) {
            gray_read_frame(&frame);
            gray_analyze_frame(&frame);
            gCal.lineValid = frame.lineValid;
            gCal.digitalMask = frame.digitalMask;
            gCal.activeSum = frame.activeSum;
            gCal.weightedError = frame.weightedError;
            gCal.turnHint = frame.turnHint;
            drive_update(&frame);
        } else {
            drive_update(NULL);
        }

        reportElapsedMs += GRAY_CONTROL_INTERVAL_MS;
        heartbeatElapsedMs += GRAY_CONTROL_INTERVAL_MS;

        if ((gStreamMode != STREAM_STOPPED) &&
            (reportElapsedMs >= GRAY_REPORT_INTERVAL_MS)) {
            reportElapsedMs = 0U;
            gray_print_frame(&frame);
        }

        if (heartbeatElapsedMs >= GRAY_HEARTBEAT_INTERVAL_MS) {
            heartbeatElapsedMs = 0U;
            uart_write_string("HB,OK,DRIVE,");
            uart_write_string(drive_mode_string(gDriveMode));
            uart_write_crlf();
        }

        delay_ms(GRAY_CONTROL_INTERVAL_MS);
    }
}
