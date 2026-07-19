#ifndef TEST_VISION_UART_H
#define TEST_VISION_UART_H

#include "../common/module_test_contract.h"

void VisionUartTest_Init(void);
ModuleTestResult VisionUartTest_RunOnce(void);
void VisionUartTest_SafeStop(void);

#endif
