#ifndef TEST_MOTOR_TB6612_H
#define TEST_MOTOR_TB6612_H

#include "motor_tb6612_interface.h"
#include "../common/module_test_contract.h"

#define MOTOR_TB6612_TEST_PWM_PERMILLE   (150U)
#define MOTOR_TB6612_MOTION_MS           (800U)
#define MOTOR_TB6612_DIRECTION_DWELL_MS  (300U)
#define MOTOR_TB6612_REQUIRED_CYCLES     (10U)

typedef enum {
    MOTOR_TEST_PHASE_IDLE = 0,
    MOTOR_TEST_PHASE_LEFT_FORWARD,
    MOTOR_TEST_PHASE_DWELL_1,
    MOTOR_TEST_PHASE_LEFT_REVERSE,
    MOTOR_TEST_PHASE_DWELL_2,
    MOTOR_TEST_PHASE_RIGHT_FORWARD,
    MOTOR_TEST_PHASE_DWELL_3,
    MOTOR_TEST_PHASE_RIGHT_REVERSE,
    MOTOR_TEST_PHASE_COMPLETE,
    MOTOR_TEST_PHASE_ABORTED
} MotorTb6612TestPhase;

typedef enum {
    MOTOR_TB6612_ERROR_NONE = 0,
    MOTOR_TB6612_ERROR_INVALID_INTERFACE = 0x3001U,
    MOTOR_TB6612_ERROR_NOT_INITIALIZED = 0x3002U,
    MOTOR_TB6612_ERROR_ARM_REQUIRED = 0x3003U,
    MOTOR_TB6612_ERROR_ESTOP_ACTIVE = 0x3004U,
    MOTOR_TB6612_ERROR_ABORTED = 0x3005U
} MotorTb6612Error;

bool MotorTb6612Test_Attach(const MotorTb6612Interface *interface);
void MotorTb6612Test_Init(void);
bool MotorTb6612Test_Arm(void);
ModuleTestResult MotorTb6612Test_RunOnce(void);
void MotorTb6612Test_SafeStop(void);
MotorTb6612TestPhase MotorTb6612Test_GetPhase(void);
uint8_t MotorTb6612Test_GetCompletedCycles(void);

#endif
