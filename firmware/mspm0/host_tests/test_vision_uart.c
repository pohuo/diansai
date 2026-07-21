#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "ModuleTests/07_vision_uart/test_vision_uart.h"

static uint32_t gNowMs;
static uint32_t gSafetyCalls;
static char gBytes[4096];
static size_t gByteCount;
static size_t gByteIndex;

static uint32_t GetTimeMs(void)
{
    return gNowMs;
}

static bool ReadByte(uint8_t *byte)
{
    if (gByteIndex >= gByteCount) {
        return false;
    }
    *byte = (uint8_t) gBytes[gByteIndex++];
    return true;
}

static void HandleLinkTimeout(void)
{
    gSafetyCalls++;
}

static void FillValidFrames(void)
{
    uint32_t sequence;

    gByteCount = 0U;
    gByteIndex = 0U;
    for (sequence = 0U; sequence < VISION_UART_REQUIRED_FRAMES;
         sequence++) {
        char body[32];
        uint32_t checksum = 0U;
        size_t index;
        int bodyLength = snprintf(body, sizeof(body),
                                  "V,%lu,1,10,20",
                                  (unsigned long) sequence);
        int frameLength;

        assert(bodyLength > 0);
        for (index = 0U; index < (size_t) bodyLength; index++) {
            checksum += (uint8_t) body[index];
        }
        checksum %= 255U;
        frameLength = snprintf(
            &gBytes[gByteCount],
            sizeof(gBytes) - gByteCount,
            "$%s,%lu*\n", body, (unsigned long) checksum);
        assert(frameLength > 0);
        gByteCount += (size_t) frameLength;
        assert(gByteCount < sizeof(gBytes));
    }
}

void HostTest_VisionUart(void)
{
    const VisionUartInterface interface = {
        GetTimeMs, ReadByte, HandleLinkTimeout
    };
    ModuleTestResult result = {0};
    VisionUartStats stats;
    uint32_t run;

    gNowMs = 0U;
    gSafetyCalls = 0U;
    assert(VisionUartTest_Attach(&interface));
    VisionUartTest_Init();

    gByteCount = (size_t) snprintf(
        gBytes, sizeof(gBytes), "$V,0,1,10,20,0*\n");
    gByteIndex = 0U;
    result = VisionUartTest_RunOnce();
    assert(result.status == MODULE_TEST_RUNNING);
    VisionUartTest_GetStats(&stats);
    assert(stats.checksum_errors == 1U);

    FillValidFrames();
    for (run = 0U; run < 100U; run++) {
        result = VisionUartTest_RunOnce();
        if (result.status == MODULE_TEST_PASSED) {
            break;
        }
    }
    assert(result.status == MODULE_TEST_PASSED);
    assert(result.measured_value ==
           (int32_t) VISION_UART_REQUIRED_FRAMES);

    gNowMs = VISION_UART_TIMEOUT_MS + 1U;
    result = VisionUartTest_RunOnce();
    assert(result.status == MODULE_TEST_FAILED);
    assert(result.error_code == VISION_UART_ERROR_TIMEOUT);
    assert(gSafetyCalls == 1U);
}