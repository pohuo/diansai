/*
 * MSPM0G3507 gray sensor debug firmware
 *
 * Stage 1 goals:
 *   - Read 8-channel gray sensor values through 74HC4051
 *   - Print raw data over UART0 (HC-05 / USB serial)
 *   - Support simple white/black calibration commands
 *   - Keep motors stopped
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ti_msp_dl_config.h"

#define GRAY_SENSOR_COUNT          (8U)
#define GRAY_SAMPLE_AVG            (4U)
#define GRAY_SETTLE_US             (8U)
#define GRAY_REPORT_INTERVAL_MS    (200U)
#define GRAY_CAL_AVG_FRAMES        (8U)
#define GRAY_DEFAULT_THRESHOLD     (1500U)
#define GRAY_ACTIVE_MIN_THRESHOLD   (50U)
#define GRAY_LINE_SUM_THRESHOLD    (180U)
#define GRAY_HEARTBEAT_FRAMES      (5U)

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

/* ------------------------------ UART helpers ----------------------------- */

static void gray_print_help(void)
{
    uart_write_string("Commands: ");
    uart_write_string("H=help, ");
    uart_write_string("R=raw stream, ");
    uart_write_string("C=calibrated stream, ");
    uart_write_string("S=stop, ");
    uart_write_string("W=capture white, ");
    uart_write_string("B=capture black, ");
    uart_write_string("T=status");
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
    if (cmd == NULL || *cmd == '\0') {
        return;
    }

    if (command_equals(cmd, "H") || command_equals(cmd, "HELP") ||
        command_equals(cmd, "?")) {
        gray_print_help();
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

    if (command_equals(cmd, "S") || command_equals(cmd, "STOP")) {
        gStreamMode = STREAM_STOPPED;
        uart_write_string("MODE,STOP");
        uart_write_crlf();
        return;
    }

    if (command_equals(cmd, "W") || command_equals(cmd, "WHITE")) {
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

        /*
         * Allow one-byte commands from phone apps or terminal tools that do
         * not append CR/LF. Keep full-word commands line-based.
         */
        if (gCmdLen == 0U) {
            switch (ascii_upper(ch)) {
                case 'H':
                case '?':
                    gray_handle_command("H");
                    continue;
                case 'R':
                    gray_handle_command("R");
                    continue;
                case 'C':
                    gray_handle_command("C");
                    continue;
                case 'S':
                    gray_handle_command("S");
                    continue;
                case 'W':
                    gray_handle_command("W");
                    continue;
                case 'B':
                    gray_handle_command("B");
                    continue;
                case 'T':
                    gray_handle_command("T");
                    continue;
                default:
                    break;
            }
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
    uint32_t tick = 0U;

    SYSCFG_DL_init();
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);

    uart_write_string("\r\nBOOT,GRAY_DEBUG\r\n");
    gray_print_help();
    uart_write_string("NOTE,HC05,9600,8N1");
    uart_write_crlf();
    uart_write_string("NOTE,5V,GND_MUST_BE_COMMON");
    uart_write_crlf();
    uart_write_string("STREAM,RAW");
    uart_write_crlf();

    while (1) {
        uart_poll_commands();

        if (gStreamMode != STREAM_STOPPED) {
            gray_read_frame(&frame);
            gray_analyze_frame(&frame);

            if (gStreamMode == STREAM_RAW || gStreamMode == STREAM_CALIBRATED) {
                gray_print_frame(&frame);
            }
        }

        delay_ms(GRAY_REPORT_INTERVAL_MS);

        if (++tick >= GRAY_HEARTBEAT_FRAMES) {
            tick = 0U;
            uart_write_string("HB,OK");
            uart_write_crlf();
        }
    }
}
