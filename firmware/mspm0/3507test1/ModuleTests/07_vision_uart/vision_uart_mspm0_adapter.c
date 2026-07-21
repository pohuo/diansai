#include "vision_uart_mspm0_adapter.h"

#include "test_vision_uart.h"
#include "../00_board_bringup/test_board_bringup.h"
#include "../03_motor_tb6612/test_motor_tb6612.h"
#include "ti_msp_dl_config.h"

#include <stdbool.h>

#define VISION_UART_RX_BUFFER_SIZE (128U)
#define VISION_UART_RX_BUFFER_MASK (VISION_UART_RX_BUFFER_SIZE - 1U)

static volatile uint8_t gRxBuffer[VISION_UART_RX_BUFFER_SIZE];
static volatile uint8_t gRxHead;
static volatile uint8_t gRxTail;
static volatile uint32_t gRxOverflowCount;

static uint32_t VisionAdapterGetTimeMs(void)
{
    return BoardBringupTest_GetUptimeMs();
}

static bool VisionAdapterReadByte(uint8_t *byte)
{
    uint8_t tail;

    if (byte == NULL) {
        return false;
    }

    tail = gRxTail;
    if (tail == gRxHead) {
        return false;
    }

    *byte = gRxBuffer[tail];
    gRxTail = (uint8_t) ((tail + 1U) & VISION_UART_RX_BUFFER_MASK);
    return true;
}

static void VisionAdapterHandleTimeout(void)
{
    MotorTb6612Test_SafeStop();
}

void VisionUartMspm0Adapter_Init(void)
{
    static const VisionUartInterface interface = {
        VisionAdapterGetTimeMs,
        VisionAdapterReadByte,
        VisionAdapterHandleTimeout
    };

    gRxHead = 0U;
    gRxTail = 0U;
    gRxOverflowCount = 0U;
    (void) VisionUartTest_Attach(&interface);
    VisionUartTest_Init();

    NVIC_ClearPendingIRQ(UART_VISION_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_VISION_INST_INT_IRQN);
}

ModuleTestResult VisionUartMspm0Adapter_RunOnce(void)
{
    ModuleTestResult result;

    if (gRxOverflowCount != 0U) {
        VisionUartTest_SafeStop();
        result.status = MODULE_TEST_FAILED;
        result.measured_value = (int32_t) gRxOverflowCount;
        result.expected_value = 0;
        result.error_code = VISION_UART_ADAPTER_ERROR_RX_OVERFLOW;
        return result;
    }

    return VisionUartTest_RunOnce();
}

void VisionUartMspm0Adapter_SafeStop(void)
{
    NVIC_DisableIRQ(UART_VISION_INST_INT_IRQN);
    VisionUartTest_SafeStop();
}

uint32_t VisionUartMspm0Adapter_GetOverflowCount(void)
{
    return gRxOverflowCount;
}

void UART_VISION_INST_IRQHandler(void)
{
    uint8_t nextHead;

    if (DL_UART_Main_getPendingInterrupt(UART_VISION_INST) !=
        DL_UART_MAIN_IIDX_RX) {
        return;
    }

    while (!DL_UART_Main_isRXFIFOEmpty(UART_VISION_INST)) {
        nextHead = (uint8_t) ((gRxHead + 1U) &
                              VISION_UART_RX_BUFFER_MASK);
        if (nextHead == gRxTail) {
            (void) DL_UART_Main_receiveData(UART_VISION_INST);
            gRxOverflowCount++;
        } else {
            gRxBuffer[gRxHead] =
                (uint8_t) DL_UART_Main_receiveData(UART_VISION_INST);
            gRxHead = nextHead;
        }
    }
}