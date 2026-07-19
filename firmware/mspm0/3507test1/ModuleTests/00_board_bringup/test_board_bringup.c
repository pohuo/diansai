#include "test_board_bringup.h"
#include "ti_msp_dl_config.h"

static volatile uint32_t gUptimeMs;
static volatile uint32_t gHeartbeatCount;
static volatile uint16_t gHeartbeatCountdownMs;
static volatile bool gInitialized;

void BoardBringupTest_Init(void)
{
    gUptimeMs             = 0U;
    gHeartbeatCount       = 0U;
    gHeartbeatCountdownMs = BOARD_BRINGUP_HEARTBEAT_HALF_PERIOD_MS;
    gInitialized          = true;

    DL_GPIO_clearPins(GPIO_STATUS_PORT, GPIO_STATUS_HEARTBEAT_PIN);
}

ModuleTestResult BoardBringupTest_RunOnce(void)
{
    uint32_t uptimeMs = gUptimeMs;
    ModuleTestResult result = {
        MODULE_TEST_BLOCKED,
        (int32_t) (uptimeMs / 1000U),
        (int32_t) (BOARD_BRINGUP_ACCEPTANCE_DURATION_MS / 1000U),
        BOARD_BRINGUP_ERROR_NOT_INITIALIZED
    };

    if (gInitialized) {
        result.status = (uptimeMs >= BOARD_BRINGUP_ACCEPTANCE_DURATION_MS)
                            ? MODULE_TEST_PASSED
                            : MODULE_TEST_RUNNING;
        result.error_code = BOARD_BRINGUP_ERROR_NONE;
    }

    return result;
}

void BoardBringupTest_SafeStop(void)
{
    gInitialized = false;
    DL_GPIO_clearPins(GPIO_STATUS_PORT, GPIO_STATUS_HEARTBEAT_PIN);
}

void BoardBringupTest_OnTick1ms(void)
{
    if (!gInitialized) {
        return;
    }

    gUptimeMs++;

    if (gHeartbeatCountdownMs > 1U) {
        gHeartbeatCountdownMs--;
        return;
    }

    gHeartbeatCountdownMs = BOARD_BRINGUP_HEARTBEAT_HALF_PERIOD_MS;
    gHeartbeatCount++;
    DL_GPIO_togglePins(GPIO_STATUS_PORT, GPIO_STATUS_HEARTBEAT_PIN);
}

uint32_t BoardBringupTest_GetUptimeMs(void)
{
    return gUptimeMs;
}
