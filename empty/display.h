#ifndef DISPLAY_H
#define DISPLAY_H

#include "gray_sensor.h"

void Display_init(void);
void Display_banner(void);
void Display_printFrame(const GraySensorFrame *frame);
void Display_printText(const char *text);

#endif
