#include "gray_sensor.h"

#include "ti_msp_dl_config.h"

static const uint16_t gDefaultThreshold[GRAY_SENSOR_CHANNELS] = {
    1042U, 1535U, 1919U, 772U, 914U, 2140U, 2682U, 2088U
};

static const int16_t gGrayWeight[GRAY_SENSOR_CHANNELS] = {
    -3500, -2500, -1500, -500, 500, 1500, 2500, 3500
};

static uint16_t gThreshold[GRAY_SENSOR_CHANNELS] = {
    1042U, 1535U, 1919U, 772U, 914U, 2140U, 2682U, 2088U
};

static uint16_t gWhiteRef[GRAY_SENSOR_CHANNELS];
static uint16_t gBlackRef[GRAY_SENSOR_CHANNELS];
static int32_t gLastError = 0;

static void gray_read_raw(uint16_t raw[GRAY_SENSOR_CHANNELS]);

static void gray_capture_reference(uint16_t ref[GRAY_SENSOR_CHANNELS])
{
    uint32_t sum[GRAY_SENSOR_CHANNELS] = { 0U };
    uint16_t sample[GRAY_SENSOR_CHANNELS];

    for (uint8_t round = 0; round < 4U; round++) {
        gray_read_raw(sample);
        for (uint8_t i = 0; i < GRAY_SENSOR_CHANNELS; i++) {
            sum[i] += sample[i];
        }
        delay_cycles(800U);
    }

    for (uint8_t i = 0; i < GRAY_SENSOR_CHANNELS; i++) {
        ref[i] = (uint16_t)(sum[i] / 4U);
    }
}

static void gray_select(uint8_t idx)
{
    if (idx & 0x01U) {
        DL_GPIO_setPins(GRAY_ADDR_A0_PORT, GRAY_ADDR_A0_PIN);
    } else {
        DL_GPIO_clearPins(GRAY_ADDR_A0_PORT, GRAY_ADDR_A0_PIN);
    }

    if (idx & 0x02U) {
        DL_GPIO_setPins(GRAY_ADDR_A1_PORT, GRAY_ADDR_A1_PIN);
    } else {
        DL_GPIO_clearPins(GRAY_ADDR_A1_PORT, GRAY_ADDR_A1_PIN);
    }

    if (idx & 0x04U) {
        DL_GPIO_setPins(GRAY_ADDR_A2_PORT, GRAY_ADDR_A2_PIN);
    } else {
        DL_GPIO_clearPins(GRAY_ADDR_A2_PORT, GRAY_ADDR_A2_PIN);
    }
}

static uint16_t gray_read_adc_once(void)
{
    uint32_t timeout = 50000U;

    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(ADC12_0_INST);

    while (timeout-- != 0U) {
        if (DL_ADC12_getEnabledInterruptStatus(
                ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) != 0U) {
            break;
        }
    }

    return DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_ADC_CH0);
}

static void gray_read_raw(uint16_t raw[GRAY_SENSOR_CHANNELS])
{
    for (uint8_t i = 0; i < GRAY_SENSOR_CHANNELS; i++) {
        uint8_t muxIndex = (uint8_t)(GRAY_SENSOR_CHANNELS - 1U - i);
        gray_select(muxIndex);
        delay_cycles(250U);
        raw[i] = gray_read_adc_once();
    }
}

static int32_t gray_calculate_error_from_pivot(const uint16_t raw[GRAY_SENSOR_CHANNELS],
    uint16_t pivot, uint8_t *maskOut)
{
    int32_t weighted = 0;
    int32_t sum = 0;
    uint8_t mask = 0U;

    for (uint8_t i = 0; i < GRAY_SENSOR_CHANNELS; i++) {
        int32_t intensity = (int32_t)pivot - (int32_t)raw[i];
        if (intensity > 0) {
            weighted += intensity * (int32_t)gGrayWeight[i];
            sum += intensity;
            mask |= (uint8_t)(1U << i);
        }
    }

    if (maskOut != NULL) {
        *maskOut = mask;
    }

    if (sum == 0) {
        return gLastError;
    }

    gLastError = weighted / sum;
    return gLastError;
}

static void gray_update_thresholds_from_refs(void)
{
    for (uint8_t i = 0; i < GRAY_SENSOR_CHANNELS; i++) {
        uint32_t hi = gWhiteRef[i];
        uint32_t lo = gBlackRef[i];
        if (hi < lo) {
            uint32_t tmp = hi;
            hi = lo;
            lo = tmp;
        }
        gThreshold[i] = (uint16_t)((hi + lo) / 2U);
    }
}

static int32_t gray_calculate_error(const uint16_t raw[GRAY_SENSOR_CHANNELS], uint8_t *maskOut)
{
    int32_t weighted = 0;
    int32_t sum = 0;
    uint8_t mask = 0U;

    for (uint8_t i = 0; i < GRAY_SENSOR_CHANNELS; i++) {
        int32_t intensity = (int32_t)gThreshold[i] - (int32_t)raw[i];
        if (intensity > 0) {
            weighted += intensity * (int32_t)gGrayWeight[i];
            sum += intensity;
            mask |= (uint8_t)(1U << i);
        }
    }

    if (maskOut != NULL) {
        *maskOut = mask;
    }

    if (sum != 0) {
        gLastError = weighted / sum;
        return gLastError;
    }

    {
        uint16_t minValue = raw[0];
        uint16_t maxValue = raw[0];

        for (uint8_t i = 1U; i < GRAY_SENSOR_CHANNELS; i++) {
            if (raw[i] < minValue) {
                minValue = raw[i];
            }
            if (raw[i] > maxValue) {
                maxValue = raw[i];
            }
        }

        if ((uint32_t)maxValue <= ((uint32_t)minValue + 40U)) {
            return gLastError;
        }

        return gray_calculate_error_from_pivot(raw,
            (uint16_t)(((uint32_t)minValue + (uint32_t)maxValue) / 2U), maskOut);
    }
}

void GraySensor_init(void)
{
    GraySensor_resetCalibration();
}

void GraySensor_resetCalibration(void)
{
    for (uint8_t i = 0; i < GRAY_SENSOR_CHANNELS; i++) {
        gThreshold[i] = gDefaultThreshold[i];
        gWhiteRef[i] = gDefaultThreshold[i];
        gBlackRef[i] = gDefaultThreshold[i];
    }
    gLastError = 0;
}

bool GraySensor_captureWhite(void)
{
    gray_capture_reference(gWhiteRef);
    gray_update_thresholds_from_refs();
    return true;
}

bool GraySensor_captureBlack(void)
{
    gray_capture_reference(gBlackRef);
    gray_update_thresholds_from_refs();
    return true;
}

void GraySensor_read(GraySensorFrame *frame)
{
    if (frame == NULL) {
        return;
    }

    gray_read_raw(frame->raw);
    for (uint8_t i = 0; i < GRAY_SENSOR_CHANNELS; i++) {
        frame->threshold[i] = gThreshold[i];
    }
    frame->mask = 0U;
    frame->error = gray_calculate_error(frame->raw, &frame->mask);
}

const uint16_t *GraySensor_getThresholds(void)
{
    return gThreshold;
}
