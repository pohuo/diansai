#ifndef TEST_BUTTON_ESTOP_H
#define TEST_BUTTON_ESTOP_H

#include "../common/module_test_contract.h"

void ButtonEstopTest_Init(void);
ModuleTestResult ButtonEstopTest_RunOnce(void);
void ButtonEstopTest_SafeStop(void);

#endif
