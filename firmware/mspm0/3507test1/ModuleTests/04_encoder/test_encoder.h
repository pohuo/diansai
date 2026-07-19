#ifndef TEST_ENCODER_H
#define TEST_ENCODER_H

#include "encoder_interface.h"
#include "../common/module_test_contract.h"

#define ENCODER_TEST_SAMPLE_MS             (20U)
#define ENCODER_TEST_MAX_DELTA_PER_SAMPLE  (10000U)
#define ENCODER_NOMINAL_COUNTS_PER_REV     (2340)

typedef enum {
    ENCODER_TEST_ERROR_NONE = 0,
    ENCODER_TEST_ERROR_INVALID_INTERFACE = 0x4001U,
    ENCODER_TEST_ERROR_NOT_INITIALIZED = 0x4002U,
    ENCODER_TEST_ERROR_READ_FAILED = 0x4003U,
    ENCODER_TEST_ERROR_DELTA_IMPLAUSIBLE = 0x4004U
} EncoderTestError;

bool EncoderTest_Attach(const EncoderInterface *interface);
void EncoderTest_Init(void);
ModuleTestResult EncoderTest_RunOnce(void);
void EncoderTest_SafeStop(void);
void EncoderTest_GetLastDeltas(int32_t *left_delta, int32_t *right_delta);
uint8_t EncoderTest_GetObservedDirectionMask(void);

#endif
