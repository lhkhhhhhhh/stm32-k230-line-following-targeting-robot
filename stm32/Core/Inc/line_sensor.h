#ifndef __LINE_SENSOR_H
#define __LINE_SENSOR_H

#include "stm32f1xx_hal.h"

/* 8-channel line sensor: X1~X8 => PA7~PA0 */
#define LINE_SENSOR_PORT        GPIOA
#define LINE_SENSOR_PINS        (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                                 GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)

#define LINE_SENSOR_COUNT       8

/* Returns bitmask: bit0=X1(PA7), ..., bit7=X8(PA0). 1=black, 0=white. */
uint8_t LineSensor_Read(void);

/* Returns sensor position: -7 (far left) to +7 (far right), 0 = centered.
   Positive = line is to the right. 128 = all-white (lost line). */
int16_t LineSensor_GetPosition(void);

/* Returns 1 if all sensors see white (line lost) */
uint8_t LineSensor_AllWhite(void);

#endif /* __LINE_SENSOR_H */
