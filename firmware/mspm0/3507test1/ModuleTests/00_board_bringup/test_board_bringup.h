#ifndef TEST_BOARD_BRINGUP_H
#define TEST_BOARD_BRINGUP_H

#include "../common/module_test_contract.h"

void BoardBringupTest_Init(void);
ModuleTestResult BoardBringupTest_RunOnce(void);
void BoardBringupTest_SafeStop(void);

#endif
