#include "display.h"

#include "ti_msp_dl_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
    OLED_WIDTH = 128,
    OLED_HEIGHT = 64,
    OLED_BUFFER_SIZE = (OLED_WIDTH * OLED_HEIGHT) / 8
};

static uint8_t gOledBuffer[OLED_BUFFER_SIZE];
static bool gOledReady = false;

static void oled_delay(void)
{
    delay_cycles(20U);
}

static void oled_scl(bool high)
{
    if (high) {
        DL_GPIO_setPins(OLED_SCL_PORT, OLED_SCL_PIN);
    } else {
        DL_GPIO_clearPins(OLED_SCL_PORT, OLED_SCL_PIN);
    }
}

static void oled_sda(bool high)
{
    if (high) {
        DL_GPIO_setPins(OLED_SDA_PORT, OLED_SDA_PIN);
    } else {
        DL_GPIO_clearPins(OLED_SDA_PORT, OLED_SDA_PIN);
    }
}

static void oled_res(bool high)
{
    if (high) {
        DL_GPIO_setPins(OLED_RES_PORT, OLED_RES_PIN);
    } else {
        DL_GPIO_clearPins(OLED_RES_PORT, OLED_RES_PIN);
    }
}

static void oled_dc(bool high)
{
    if (high) {
        DL_GPIO_setPins(OLED_DC_PORT, OLED_DC_PIN);
    } else {
        DL_GPIO_clearPins(OLED_DC_PORT, OLED_DC_PIN);
    }
}

static void oled_write_bit(bool bit)
{
    oled_sda(bit);
    oled_delay();
    oled_scl(true);
    oled_delay();
    oled_scl(false);
}

static void oled_write_byte(uint8_t data, bool isData)
{
    oled_dc(isData);
    oled_scl(false);

    for (uint8_t bit = 0; bit < 8U; bit++) {
        oled_write_bit((data & 0x80U) != 0U);
        data <<= 1U;
    }

    oled_sda(true);
    oled_delay();
}

static void oled_cmd(uint8_t cmd)
{
    oled_write_byte(cmd, false);
}

static void oled_data(uint8_t data)
{
    oled_write_byte(data, true);
}

static void oled_clear_buffer(void)
{
    memset(gOledBuffer, 0, sizeof(gOledBuffer));
}

static void oled_set_pixel(uint8_t x, uint8_t y, bool on)
{
    uint16_t index;
    uint8_t mask;

    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) {
        return;
    }

    index = (uint16_t)x + ((uint16_t)(y >> 3U) * OLED_WIDTH);
    mask = (uint8_t)(1U << (y & 7U));
    if (on) {
        gOledBuffer[index] |= mask;
    } else {
        gOledBuffer[index] &= (uint8_t)~mask;
    }
}

static void oled_draw_vline(uint8_t x, uint8_t y0, uint8_t y1)
{
    uint8_t y;

    if (x >= OLED_WIDTH) {
        return;
    }

    if (y0 > y1) {
        uint8_t tmp = y0;
        y0 = y1;
        y1 = tmp;
    }

    for (y = y0; y <= y1; y++) {
        oled_set_pixel(x, y, true);
    }
}

static void oled_draw_filled_rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t x;

    if (x0 > x1) {
        uint8_t tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    if (y0 > y1) {
        uint8_t tmp = y0;
        y0 = y1;
        y1 = tmp;
    }

    for (x = x0; x <= x1; x++) {
        for (uint8_t y = y0; y <= y1; y++) {
            oled_set_pixel(x, y, true);
        }
    }
}

static void oled_flush(void)
{
    for (uint8_t page = 0U; page < 8U; page++) {
        uint16_t base = (uint16_t)page * OLED_WIDTH;

        oled_cmd((uint8_t)(0xB0U | page));
        oled_cmd(0x00U);
        oled_cmd(0x10U);

        for (uint16_t x = 0U; x < OLED_WIDTH; x++) {
            oled_data(gOledBuffer[base + x]);
        }
    }
}

static void oled_init_hw(void)
{
    oled_scl(false);
    oled_sda(false);
    oled_dc(false);
    oled_res(true);
    delay_cycles(320000U);
    oled_res(false);
    delay_cycles(320000U);
    oled_res(true);
    delay_cycles(320000U);
}

static void oled_init_seq(void)
{
    oled_cmd(0xAEU);
    oled_cmd(0xD5U);
    oled_cmd(0x80U);
    oled_cmd(0xA8U);
    oled_cmd(0x3FU);
    oled_cmd(0xD3U);
    oled_cmd(0x00U);
    oled_cmd(0x40U);
    oled_cmd(0x8DU);
    oled_cmd(0x14U);
    oled_cmd(0x20U);
    oled_cmd(0x00U);
    oled_cmd(0xA1U);
    oled_cmd(0xC8U);
    oled_cmd(0xDAU);
    oled_cmd(0x12U);
    oled_cmd(0x81U);
    oled_cmd(0x7FU);
    oled_cmd(0xD9U);
    oled_cmd(0xF1U);
    oled_cmd(0xDBU);
    oled_cmd(0x40U);
    oled_cmd(0xA4U);
    oled_cmd(0xA6U);
    oled_cmd(0xAFU);
}

static uint8_t scale_level(uint16_t raw, uint16_t threshold)
{
    uint32_t level;

    if (threshold == 0U) {
        return 0U;
    }
    if (raw >= threshold) {
        return 0U;
    }

    level = ((uint32_t)(threshold - raw) * 48U) / threshold;
    if (level > 48U) {
        level = 48U;
    }

    return (uint8_t)level;
}

static void oled_draw_frame(const GraySensorFrame *frame)
{
    int32_t cursorPos;
    uint8_t cursorX;

    oled_clear_buffer();

    /* A thin top rail gives a quick "alive" cue after reset. */
    oled_draw_vline(63U, 0U, 7U);
    oled_draw_vline(64U, 0U, 7U);

    for (uint8_t i = 0U; i < GRAY_SENSOR_CHANNELS; i++) {
        uint8_t barHeight = scale_level(frame->raw[i], frame->threshold[i]);
        uint8_t x0 = (uint8_t)(4U + (i * 15U));
        uint8_t x1 = (uint8_t)(x0 + 9U);
        uint8_t y1 = 63U;
        uint8_t y0 = (uint8_t)(y1 - barHeight);

        if (barHeight == 0U) {
            y0 = 63U;
        }

        oled_draw_filled_rect(x0, y0, x1, y1);
    }

    cursorPos = 64 + (frame->error / 72);
    if (cursorPos < 4) {
        cursorPos = 4;
    }
    if (cursorPos > 123) {
        cursorPos = 123;
    }
    cursorX = (uint8_t)cursorPos;
    oled_draw_filled_rect((uint8_t)(cursorX - 2U), 2U, (uint8_t)(cursorX + 2U), 6U);

    oled_flush();
}

void Display_init(void)
{
    oled_init_hw();
    oled_init_seq();
    oled_clear_buffer();
    oled_flush();
    gOledReady = true;
}

void Display_banner(void)
{
    (void)0;
}

void Display_printText(const char *text)
{
    (void)text;
}

void Display_printFrame(const GraySensorFrame *frame)
{
    if (frame == NULL) {
        return;
    }

    if (gOledReady) {
        oled_draw_frame(frame);
    }
}
