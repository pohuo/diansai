#ifndef TEST_TOF_STP23L_H
#define TEST_TOF_STP23L_H

#include "tof_stp23l_interface.h"
#include "../common/module_test_contract.h"

#define TOF_STP23L_MIN_DISTANCE_MM      (30U)
#define TOF_STP23L_MAX_DISTANCE_MM      (7500U)
#define TOF_STP23L_NOMINAL_RATE_HZ      (120U)
#define TOF_STP23L_TIMEOUT_MS           (100U)
#define TOF_STP23L_REQUIRED_SAMPLES     (120U)
#define TOF_STP23L_MAX_SAMPLES_PER_RUN  (16U)

typedef enum {
    TOF_STP23L_ERROR_NONE = 0,
    TOF_STP23L_ERROR_INVALID_INTERFACE = 0x6001U,
    TOF_STP23L_ERROR_NOT_INITIALIZED = 0x6002U,
    TOF_STP23L_ERROR_REFERENCE_REQUIRED = 0x6003U,
    TOF_STP23L_ERROR_INVALID_SAMPLE = 0x6004U,
    TOF_STP23L_ERROR_OUT_OF_RANGE = 0x6005U,
    TOF_STP23L_ERROR_OUT_OF_TOLERANCE = 0x6006U,
    TOF_STP23L_ERROR_TIMEOUT = 0x6007U
} TofStp23lError;

typedef struct {
    uint32_t accepted_samples;
    uint32_t consecutive_good_samples;
    uint32_t invalid_samples;
    uint32_t out_of_range_samples;
    uint32_t out_of_tolerance_samples;
    uint32_t timeout_events;
    uint16_t minimum_mm;
    uint16_t maximum_mm;
    uint32_t average_mm;
} TofStp23lStats;

bool TofStp23lTest_Attach(const TofStp23lInterface *interface);
void TofStp23lTest_Init(void);
bool TofStp23lTest_SetReference(uint16_t distance_mm,
                               uint16_t tolerance_mm);
ModuleTestResult TofStp23lTest_RunOnce(void);
void TofStp23lTest_SafeStop(void);
void TofStp23lTest_GetStats(TofStp23lStats *stats);

#endif