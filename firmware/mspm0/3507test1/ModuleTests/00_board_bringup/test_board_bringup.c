#include "test_board_bringup.h"

void BoardBringupTest_Init(void) {}

ModuleTestResult BoardBringupTest_RunOnce(void)
{
    ModuleTestResult result = {MODULE_TEST_BLOCKED, 0, 0, 0};
    return result;
}

void BoardBringupTest_SafeStop(void) {}
