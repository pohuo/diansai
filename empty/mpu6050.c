#include "mpu6050.h"

#include "ti_msp_dl_config.h"

#define MPU6050_ADDR      (0x68U)
#define REG_SMPLRT_DIV    (0x19U)
#define REG_CONFIG        (0x1AU)
#define REG_GYRO_CONFIG   (0x1BU)
#define REG_ACCEL_CONFIG  (0x1CU)
#define REG_ACCEL_XOUT_H  (0x3BU)
#define REG_PWR_MGMT_1    (0x6BU)
#define REG_WHO_AM_I      (0x75U)

#define I2C_TIMEOUT       (100000U)

static bool gMpuReady = false;

static void mpu_delay_ms(uint32_t ms)
{
    delay_cycles((CPUCLK_FREQ / 1000U) * ms);
}

static bool i2c_wait_idle(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while ((DL_I2C_getControllerStatus(I2C_0_INST) &
            DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) {
        if (timeout-- == 0U) {
            return false;
        }
    }

    return true;
}

static bool i2c_wait_done(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while ((DL_I2C_getControllerStatus(I2C_0_INST) &
            DL_I2C_CONTROLLER_STATUS_BUSY_BUS) != 0U) {
        if (DL_I2C_getControllerStatus(I2C_0_INST) &
            DL_I2C_CONTROLLER_STATUS_ERROR) {
            return false;
        }
        if (timeout-- == 0U) {
            return false;
        }
    }

    if (DL_I2C_getControllerStatus(I2C_0_INST) &
        DL_I2C_CONTROLLER_STATUS_ERROR) {
        return false;
    }

    return true;
}

static bool i2c_write_bytes(const uint8_t *buf, uint8_t len)
{
    if ((buf == NULL) || (len == 0U)) {
        return false;
    }
    if (!i2c_wait_idle()) {
        return false;
    }

    DL_I2C_flushControllerTXFIFO(I2C_0_INST);
    DL_I2C_flushControllerRXFIFO(I2C_0_INST);
    DL_I2C_fillControllerTXFIFO(I2C_0_INST, buf, len);
    DL_I2C_startControllerTransfer(
        I2C_0_INST, MPU6050_ADDR, DL_I2C_CONTROLLER_DIRECTION_TX, len);

    return i2c_wait_done();
}

static bool i2c_read_bytes(uint8_t reg, uint8_t *buf, uint8_t len)
{
    if ((buf == NULL) || (len == 0U)) {
        return false;
    }

    if (!i2c_write_bytes(&reg, 1U)) {
        return false;
    }
    if (!i2c_wait_idle()) {
        return false;
    }

    DL_I2C_flushControllerRXFIFO(I2C_0_INST);
    DL_I2C_startControllerTransfer(
        I2C_0_INST, MPU6050_ADDR, DL_I2C_CONTROLLER_DIRECTION_RX, len);

    for (uint8_t i = 0U; i < len; i++) {
        uint32_t timeout = I2C_TIMEOUT;
        while (DL_I2C_isControllerRXFIFOEmpty(I2C_0_INST)) {
            if (DL_I2C_getControllerStatus(I2C_0_INST) &
                DL_I2C_CONTROLLER_STATUS_ERROR) {
                return false;
            }
            if (timeout-- == 0U) {
                return false;
            }
        }
        buf[i] = DL_I2C_receiveControllerData(I2C_0_INST);
    }

    return i2c_wait_done();
}

static bool mpu_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t packet[2] = { reg, value };
    return i2c_write_bytes(packet, 2U);
}

void Mpu6050_init(void)
{
    uint8_t whoAmI = 0U;

    gMpuReady = false;
    if (!mpu_write_reg(REG_PWR_MGMT_1, 0x00U)) {
        return;
    }
    mpu_delay_ms(100U);

    if (!i2c_read_bytes(REG_WHO_AM_I, &whoAmI, 1U)) {
        return;
    }
    if (whoAmI != 0x68U) {
        return;
    }

    if (!mpu_write_reg(REG_SMPLRT_DIV, 0x07U)) {
        return;
    }
    if (!mpu_write_reg(REG_CONFIG, 0x03U)) {
        return;
    }
    if (!mpu_write_reg(REG_GYRO_CONFIG, 0x08U)) {
        return;
    }
    if (!mpu_write_reg(REG_ACCEL_CONFIG, 0x08U)) {
        return;
    }

    gMpuReady = true;
}

bool Mpu6050_isReady(void)
{
    return gMpuReady;
}

bool Mpu6050_readRaw(Mpu6050Raw *data)
{
    uint8_t raw[14];

    if ((data == NULL) || !gMpuReady) {
        return false;
    }

    if (!i2c_read_bytes(REG_ACCEL_XOUT_H, raw, sizeof(raw))) {
        return false;
    }

    data->ax = (int16_t)((((uint16_t)raw[0]) << 8) | raw[1]);
    data->ay = (int16_t)((((uint16_t)raw[2]) << 8) | raw[3]);
    data->az = (int16_t)((((uint16_t)raw[4]) << 8) | raw[5]);
    data->temp = (int16_t)((((uint16_t)raw[6]) << 8) | raw[7]);
    data->gx = (int16_t)((((uint16_t)raw[8]) << 8) | raw[9]);
    data->gy = (int16_t)((((uint16_t)raw[10]) << 8) | raw[11]);
    data->gz = (int16_t)((((uint16_t)raw[12]) << 8) | raw[13]);

    return true;
}
