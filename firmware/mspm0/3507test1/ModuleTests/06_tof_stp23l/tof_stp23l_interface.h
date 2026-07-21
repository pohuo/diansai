#ifndef TOF_STP23L_INTERFACE_H
#define TOF_STP23L_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The STP-23L public datasheet does not define its binary frame layout.
 * poll_sample must therefore return a sample decoded by a verified adapter.
 */
typedef struct {
    uint16_t distance_mm;
    bool valid;
} TofStp23lSample;

typedef struct {
    uint32_t (*get_time_ms)(void);
    bool (*poll_sample)(TofStp23lSample *sample);
    void (*handle_timeout)(void);
} TofStp23lInterface;

#endif