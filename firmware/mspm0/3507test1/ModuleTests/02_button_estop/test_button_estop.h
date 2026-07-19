#ifndef TEST_BUTTON_ESTOP_H
#define TEST_BUTTON_ESTOP_H

#include "button_estop_interface.h"
#include "../common/module_test_contract.h"

#define BUTTON_ESTOP_DEBOUNCE_MS       (20U)
#define BUTTON_ESTOP_REQUIRED_PRESSES  (20U)

typedef enum {
    BUTTON_ESTOP_ERROR_NONE = 0,
    BUTTON_ESTOP_ERROR_INVALID_INTERFACE = 0x2001U,
    BUTTON_ESTOP_ERROR_NOT_INITIALIZED = 0x2002U
} ButtonEstopError;

bool ButtonEstopTest_Attach(const ButtonEstopInterface *interface);
void ButtonEstopTest_Init(void);
ModuleTestResult ButtonEstopTest_RunOnce(void);
void ButtonEstopTest_SafeStop(void);
uint16_t ButtonEstopTest_GetPressCount(void);
bool ButtonEstopTest_WasEstopObserved(void);

#endif
