#include "test_line_sensor.h"

void LineSensorTest_Init(void) {}

ModuleTestResult LineSensorTest_RunOnce(void)
{
    ModuleTestResult result = {MODULE_TEST_BLOCKED, 0, 4, 0};
    return result;
}

void LineSensorTest_SafeStop(void) {}
