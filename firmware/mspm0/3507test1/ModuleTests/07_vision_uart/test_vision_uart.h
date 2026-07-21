#ifndef TEST_VISION_UART_H
#define TEST_VISION_UART_H

#include "vision_uart_interface.h"
#include "../common/module_test_contract.h"

#define VISION_UART_FRAME_MAX_LENGTH  (48U)
#define VISION_UART_TIMEOUT_MS         (200U)
#define VISION_UART_MAX_BYTES_PER_RUN  (64U)
#define VISION_UART_REQUIRED_FRAMES    (100U)
#define VISION_UART_IMAGE_WIDTH        (320U)
#define VISION_UART_IMAGE_HEIGHT       (240U)

typedef enum {
    VISION_UART_ERROR_NONE = 0,
    VISION_UART_ERROR_INVALID_INTERFACE = 0x7001U,
    VISION_UART_ERROR_NOT_INITIALIZED = 0x7002U,
    VISION_UART_ERROR_FORMAT = 0x7003U,
    VISION_UART_ERROR_CHECKSUM = 0x7004U,
    VISION_UART_ERROR_RANGE = 0x7005U,
    VISION_UART_ERROR_TIMEOUT = 0x7006U
} VisionUartError;

typedef struct {
    uint8_t sequence;
    bool valid;
    uint16_t x;
    uint16_t y;
    uint32_t received_at_ms;
} VisionUartTarget;

typedef struct {
    uint32_t accepted_frames;
    uint32_t consecutive_good_frames;
    uint32_t checksum_errors;
    uint32_t format_errors;
    uint32_t range_errors;
    uint32_t sequence_errors;
    uint32_t timeout_events;
} VisionUartStats;

bool VisionUartTest_Attach(const VisionUartInterface *interface);
void VisionUartTest_Init(void);
ModuleTestResult VisionUartTest_RunOnce(void);
void VisionUartTest_SafeStop(void);
bool VisionUartTest_GetLatestTarget(VisionUartTarget *target);
void VisionUartTest_GetStats(VisionUartStats *stats);

#endif
