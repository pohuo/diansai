#include "test_button_estop.h"

void ButtonEstopTest_Init(void) {}

ModuleTestResult ButtonEstopTest_RunOnce(void)
{
    ModuleTestResult result = {MODULE_TEST_BLOCKED, 0, 0, 0};
    return result;
}

void ButtonEstopTest_SafeStop(void) {}
