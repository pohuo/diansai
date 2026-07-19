#ifndef TEST_MOTOR_TB6612_H
#define TEST_MOTOR_TB6612_H

#include "../common/module_test_contract.h"

void MotorTb6612Test_Init(void);
ModuleTestResult MotorTb6612Test_RunOnce(void);
void MotorTb6612Test_SafeStop(void);

#endif
