#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t temp;
    int16_t gx;
    int16_t gy;
    int16_t gz;
} Mpu6050Raw;

void Mpu6050_init(void);
bool Mpu6050_isReady(void);
bool Mpu6050_readRaw(Mpu6050Raw *data);

#endif
