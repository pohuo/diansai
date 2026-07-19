#ifndef MODULE_TEST_CONTRACT_H
#define MODULE_TEST_CONTRACT_H

#include <stdint.h>

typedef enum {
    MODULE_TEST_NOT_RUN = 0,
    MODULE_TEST_RUNNING,
    MODULE_TEST_PASSED,
    MODULE_TEST_FAILED,
    MODULE_TEST_BLOCKED
} ModuleTestStatus;

typedef struct {
    ModuleTestStatus status;
    int32_t measured_value;
    int32_t expected_value;
    uint32_t error_code;
} ModuleTestResult;

#endif
