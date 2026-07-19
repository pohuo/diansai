#ifndef TEST_GIMBAL_H
#define TEST_GIMBAL_H

#include "../common/module_test_contract.h"

void GimbalTest_Init(void);
ModuleTestResult GimbalTest_RunOnce(void);
void GimbalTest_SafeStop(void);

#endif
