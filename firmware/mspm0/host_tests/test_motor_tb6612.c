#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ModuleTests/03_motor_tb6612/test_motor_tb6612.h"

static uint32_t gNowMs;
static bool gEstopAsserted;
static bool gStandbyEnabled;
static MotorTb6612Direction gDirection[2];
static uint16_t gPwmPermille[2];

static uint32_t GetTimeMs(void)
{
    return gNowMs;
}

static bool IsEstopAsserted(void)
{
    return gEstopAsserted;
}

static void SetStandby(bool enabled)
{
    gStandbyEnabled = enabled;
}

static void SetMotorOutput(MotorTb6612Channel channel,
                           MotorTb6612Direction direction,
                           uint16_t pwmPermille)
{
    gDirection[(uint32_t) channel] = direction;
    gPwmPermille[(uint32_t) channel] = pwmPermille;
}

static ModuleTestResult AdvanceMotorTest(uint32_t elapsedMs)
{
    gNowMs += elapsedMs;
    return MotorTb6612Test_RunOnce();
}

void HostTest_MotorTb6612(void)
{
    const MotorTb6612Interface interface = {
        GetTimeMs, IsEstopAsserted, SetStandby, SetMotorOutput
    };
    ModuleTestResult result = {0};
    uint32_t cycle;

    gNowMs = 0U;
    gEstopAsserted = false;
    gStandbyEnabled = false;
    assert(MotorTb6612Test_Attach(&interface));
    MotorTb6612Test_Init();

    for (cycle = 0U; cycle < MOTOR_TB6612_REQUIRED_CYCLES; cycle++) {
        assert(MotorTb6612Test_Arm());
        assert(gStandbyEnabled);
        assert(gDirection[MOTOR_TB6612_LEFT] == MOTOR_TB6612_FORWARD);
        assert(gPwmPermille[MOTOR_TB6612_LEFT] ==
               MOTOR_TB6612_TEST_PWM_PERMILLE);

        result = AdvanceMotorTest(MOTOR_TB6612_MOTION_MS);
        result = AdvanceMotorTest(MOTOR_TB6612_DIRECTION_DWELL_MS);
        assert(gDirection[MOTOR_TB6612_LEFT] == MOTOR_TB6612_REVERSE);
        result = AdvanceMotorTest(MOTOR_TB6612_MOTION_MS);
        result = AdvanceMotorTest(MOTOR_TB6612_DIRECTION_DWELL_MS);
        assert(gDirection[MOTOR_TB6612_RIGHT] == MOTOR_TB6612_FORWARD);
        result = AdvanceMotorTest(MOTOR_TB6612_MOTION_MS);
        result = AdvanceMotorTest(MOTOR_TB6612_DIRECTION_DWELL_MS);
        assert(gDirection[MOTOR_TB6612_RIGHT] == MOTOR_TB6612_REVERSE);
        result = AdvanceMotorTest(MOTOR_TB6612_MOTION_MS);
        assert(!gStandbyEnabled);
        assert(gPwmPermille[MOTOR_TB6612_LEFT] == 0U);
        assert(gPwmPermille[MOTOR_TB6612_RIGHT] == 0U);
    }
    assert(result.status == MODULE_TEST_PASSED);

    MotorTb6612Test_Init();
    assert(MotorTb6612Test_Arm());
    gEstopAsserted = true;
    result = MotorTb6612Test_RunOnce();
    assert(result.status == MODULE_TEST_FAILED);
    assert(result.error_code == MOTOR_TB6612_ERROR_ESTOP_ACTIVE);
    assert(!gStandbyEnabled);
    assert(gPwmPermille[MOTOR_TB6612_LEFT] == 0U);
    assert(gPwmPermille[MOTOR_TB6612_RIGHT] == 0U);
}