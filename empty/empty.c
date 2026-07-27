#include "ti_msp_dl_config.h"

#include "display.h"
#include "gray_sensor.h"
#include "line_follow.h"
#include "motor_drv8701.h"
#include "mpu6050.h"

#include <stdbool.h>
#include <stdint.h>

#define CONTROL_INTERVAL_MS   (20U)
#define OLED_INTERVAL_MS      (120U)
#define UART_INTERVAL_MS      (240U)

static GraySensorFrame gFrame;
static LineFollowCommand gFollowCommand;
static Mpu6050Raw gImuData;
static uint32_t gOledElapsedMs = 0U;
static uint32_t gUartElapsedMs = 0U;
static uint8_t gUartPhase = 0U;

static void uart_tx_char(char ch)
{
    DL_UART_Main_transmitDataBlocking(UART_0_INST, (uint8_t)ch);
}

static void uart_tx_str(const char *text)
{
    if (text == NULL) {
        return;
    }

    while (*text != '\0') {
        uart_tx_char(*text++);
    }
}

static void uart_tx_u32(uint32_t value)
{
    char digits[10];
    uint8_t count = 0U;

    if (value == 0U) {
        uart_tx_char('0');
        return;
    }

    while ((value > 0U) && (count < sizeof(digits))) {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (count > 0U) {
        uart_tx_char(digits[--count]);
    }
}

static void uart_tx_i32(int32_t value)
{
    if (value < 0) {
        uart_tx_char('-');
        uart_tx_u32((uint32_t)(-(int64_t)value));
    } else {
        uart_tx_u32((uint32_t)value);
    }
}

static void uart_tx_u16_list(const uint16_t *values, uint8_t count)
{
    uint8_t i;

    if (values == NULL) {
        return;
    }

    for (i = 0U; i < count; i++) {
        if (i != 0U) {
            uart_tx_char(',');
        }
        uart_tx_u32(values[i]);
    }
}

static void debug_uart_print_frame(const GraySensorFrame *frame,
    const LineFollowCommand *command)
{
    if ((frame == NULL) || (command == NULL)) {
        return;
    }

    if (gUartPhase == 0U) {
        uart_tx_str("RAW,");
        uart_tx_u16_list(frame->raw, GRAY_SENSOR_CHANNELS);
        uart_tx_str("\r\n");
    } else {
        uart_tx_str("META,THR,");
        uart_tx_u16_list(frame->threshold, GRAY_SENSOR_CHANNELS);
        uart_tx_str(";M=");
        uart_tx_u32(frame->mask);
        uart_tx_str(";RE=");
        uart_tx_i32(frame->error);
        uart_tx_str(";CE=");
        uart_tx_i32(command->error);
        uart_tx_str(";CB=");
        uart_tx_i32(LineFollow_getCenterBias());
        uart_tx_str(";LT=");
        uart_tx_i32(LineFollow_getLeftTrim());
        uart_tx_str(";RT=");
        uart_tx_i32(LineFollow_getRightTrim());
        uart_tx_str(";L=");
        uart_tx_i32(command->left);
        uart_tx_str(";R=");
        uart_tx_i32(command->right);
        uart_tx_str(";LOST=");
        uart_tx_u32(command->lineLost ? 1U : 0U);
        uart_tx_str("\r\n");
    }

    gUartPhase ^= 1U;
}

static void delay_ms(uint32_t milliseconds)
{
    delay_cycles((CPUCLK_FREQ / 1000U) * milliseconds);
}

int main(void)
{
    SYSCFG_DL_init();
    __enable_irq();

    Display_init();
    GraySensor_init();
    LineFollow_init();
    Motor_init();
    Mpu6050_init();

    Display_banner();
    Motor_stop();
    delay_ms(300U);

    while (1) {
        GraySensor_read(&gFrame);
        LineFollow_compute(&gFrame, &gFollowCommand);
        Motor_drive(gFollowCommand.left, gFollowCommand.right);

        if (Mpu6050_isReady()) {
            (void)Mpu6050_readRaw(&gImuData);
        }

        gOledElapsedMs += CONTROL_INTERVAL_MS;
        if (gOledElapsedMs >= OLED_INTERVAL_MS) {
            gOledElapsedMs = 0U;
            Display_printFrame(&gFrame);
        }

        gUartElapsedMs += CONTROL_INTERVAL_MS;
        if (gUartElapsedMs >= UART_INTERVAL_MS) {
            gUartElapsedMs = 0U;
            debug_uart_print_frame(&gFrame, &gFollowCommand);
        }

        delay_ms(CONTROL_INTERVAL_MS);
    }
}
