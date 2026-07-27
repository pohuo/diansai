#ifndef MOTOR_TB6612_H
#define MOTOR_TB6612_H

#include <stdint.h>

#define MOTOR_COMMAND_MAX (1000)

void Motor_init(void);
void Motor_stop(void);
void Motor_drive(int leftCommand, int rightCommand);
int16_t Motor_getLeftCommand(void);
int16_t Motor_getRightCommand(void);

#endif
