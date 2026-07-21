#ifndef VISION_UART_MSPM0_ADAPTER_H
#define VISION_UART_MSPM0_ADAPTER_H

#include <stdint.h>

#include "../common/module_test_contract.h"

#define VISION_UART_ADAPTER_ERROR_RX_OVERFLOW (0x7101U)

void VisionUartMspm0Adapter_Init(void);
ModuleTestResult VisionUartMspm0Adapter_RunOnce(void);
void VisionUartMspm0Adapter_SafeStop(void);
uint32_t VisionUartMspm0Adapter_GetOverflowCount(void);

#endif