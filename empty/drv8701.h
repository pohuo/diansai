#ifndef DRV8701_H
#define DRV8701_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Signed motor commands use a normalized range:
 *   +1000 = forward, full duty
 *       0 = brake
 *   -1000 = reverse, full duty
 *
 * DRV8701E uses PH/EN control. EN is driven by PWM and PH selects the
 * electrical direction. With EN low the bridge uses brake/slow decay; this
 * board does not expose nSLEEP, so a true coast/high-impedance stop is not
 * available through this interface.
 */
#define DRV8701_COMMAND_MAX        (1000)
#define DRV8701_PWM_FREQUENCY_HZ   (20000U)

/*
 * Change either value to false if positive commands turn that wheel
 * backwards. Do not also swap the same motor's output wires.
 */
#define DRV8701_LEFT_FORWARD_PH_HIGH   (true)
#define DRV8701_RIGHT_FORWARD_PH_HIGH  (true)

void DRV8701_init(void);
void DRV8701_brake(void);
void DRV8701_setMotors(int16_t leftCommand, int16_t rightCommand);
int16_t DRV8701_getLeftCommand(void);
int16_t DRV8701_getRightCommand(void);

#endif