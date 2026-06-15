#include "line_sensor.h"

/* Read all 8 sensors, return bitmask: bit0=PA7(X1) ... bit7=PA0(X8) */
uint8_t LineSensor_Read(void)
{
    uint16_t port = LINE_SENSOR_PORT->IDR;
    uint8_t result = 0;

    /* PA7 -> bit0 (X1, leftmost), PA0 -> bit7 (X8, rightmost) */
    if (port & GPIO_PIN_7) result |= (1 << 0);
    if (port & GPIO_PIN_6) result |= (1 << 1);
    if (port & GPIO_PIN_5) result |= (1 << 2);
    if (port & GPIO_PIN_4) result |= (1 << 3);
    if (port & GPIO_PIN_3) result |= (1 << 4);
    if (port & GPIO_PIN_2) result |= (1 << 5);
    if (port & GPIO_PIN_1) result |= (1 << 6);
    if (port & GPIO_PIN_0) result |= (1 << 7);

    /* Invert: sensor outputs 0 on black line, 1 on white surface.
       We want: 1=black(line), 0=white(background) */
    return ~result;
}

/* Weighted average position. Returns -7..+7 for normal case, 128 if all white. */
int16_t LineSensor_GetPosition(void)
{
    uint8_t data = LineSensor_Read();

    /* All white (no line detected) */
    if (data == 0) return 128;

    int16_t sum = 0;
    int8_t count = 0;

    /* Sensor weights: bit0(X1,PA7) = -7 ... bit7(X8,PA0) = +7 */
    const int8_t weights[8] = {-7, -5, -3, -1, 1, 3, 5, 7};

    for (int i = 0; i < 8; i++) {
        if (data & (1 << i)) {
            sum += weights[i];
            count++;
        }
    }

    if (count == 0) return 128;
    return (int8_t)(sum / count);
}

uint8_t LineSensor_AllWhite(void)
{
    return (LineSensor_Read() == 0) ? 1 : 0;
}
