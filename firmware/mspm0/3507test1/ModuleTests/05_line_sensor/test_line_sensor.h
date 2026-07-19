#ifndef TEST_LINE_SENSOR_H
#define TEST_LINE_SENSOR_H

#include "../common/module_test_contract.h"

void LineSensorTest_Init(void);
ModuleTestResult LineSensorTest_RunOnce(void);
void LineSensorTest_SafeStop(void);

#endif
