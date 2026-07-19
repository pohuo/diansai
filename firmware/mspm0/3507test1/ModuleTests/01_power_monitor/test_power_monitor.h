#ifndef TEST_POWER_MONITOR_H
#define TEST_POWER_MONITOR_H

#include "../common/module_test_contract.h"

void PowerMonitorTest_Init(void);
ModuleTestResult PowerMonitorTest_RunOnce(void);
void PowerMonitorTest_SafeStop(void);

#endif
