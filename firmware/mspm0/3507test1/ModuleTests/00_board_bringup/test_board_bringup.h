#ifndef TEST_BOARD_BRINGUP_H
#define TEST_BOARD_BRINGUP_H

#include "../common/module_test_contract.h"

#define BOARD_BRINGUP_HEARTBEAT_HALF_PERIOD_MS (500U)
#define BOARD_BRINGUP_ACCEPTANCE_DURATION_MS   (30UL * 60UL * 1000UL)

typedef enum {
    BOARD_BRINGUP_ERROR_NONE = 0,
    BOARD_BRINGUP_ERROR_NOT_INITIALIZED = 0xB001U
} BoardBringupError;

void BoardBringupTest_Init(void);
ModuleTestResult BoardBringupTest_RunOnce(void);
void BoardBringupTest_SafeStop(void);
void BoardBringupTest_OnTick1ms(void);
uint32_t BoardBringupTest_GetUptimeMs(void);

#endif
