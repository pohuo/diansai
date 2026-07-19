#include "test_vision_uart.h"

void VisionUartTest_Init(void) {}

ModuleTestResult VisionUartTest_RunOnce(void)
{
    ModuleTestResult result = {MODULE_TEST_BLOCKED, 0, 100, 0};
    return result;
}

void VisionUartTest_SafeStop(void) {}
