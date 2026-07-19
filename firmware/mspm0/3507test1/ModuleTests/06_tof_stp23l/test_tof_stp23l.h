#ifndef TEST_TOF_STP23L_H
#define TEST_TOF_STP23L_H

#include "../common/module_test_contract.h"

void TofStp23lTest_Init(void);
ModuleTestResult TofStp23lTest_RunOnce(void);
void TofStp23lTest_SafeStop(void);

#endif
