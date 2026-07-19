#include "test_encoder.h"

void EncoderTest_Init(void) {}

ModuleTestResult EncoderTest_RunOnce(void)
{
    ModuleTestResult result = {MODULE_TEST_BLOCKED, 0, 2340, 0};
    return result;
}

void EncoderTest_SafeStop(void) {}
