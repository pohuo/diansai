#ifndef ENCODER_INTERFACE_H
#define ENCODER_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    ENCODER_LEFT = 0,
    ENCODER_RIGHT = 1
} EncoderChannel;

/*
 * read_count returns the signed accumulated quadrature count. The adapter may
 * use a timer/QEI peripheral or an interrupt-based decoder.
 */
typedef struct {
    uint32_t (*get_time_ms)(void);
    bool (*read_count)(EncoderChannel channel, int32_t *count);
    void (*force_actuators_safe)(void);
} EncoderInterface;

#endif
