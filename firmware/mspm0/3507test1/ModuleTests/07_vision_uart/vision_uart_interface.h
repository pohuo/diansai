#ifndef VISION_UART_INTERFACE_H
#define VISION_UART_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * read_byte is non-blocking: return true only when one byte was written.
 * The board adapter owns the UART instance, RX ring buffer and SysConfig pins.
 */
typedef struct {
    uint32_t (*get_time_ms)(void);
    bool (*read_byte)(uint8_t *byte);
    void (*handle_link_timeout)(void);
} VisionUartInterface;

#endif
