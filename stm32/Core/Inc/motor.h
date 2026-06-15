#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f1xx_hal.h"

/* TB6612 pin definitions */
/* Motor A (left) */
#define MOTOR_A_PWMA_PORT       GPIOA
#define MOTOR_A_PWMA_PIN        GPIO_PIN_8      /* TIM1_CH1 */
#define MOTOR_A_AIN1_PORT       GPIOA
#define MOTOR_A_AIN1_PIN        GPIO_PIN_9
#define MOTOR_A_AIN2_PORT       GPIOA
#define MOTOR_A_AIN2_PIN        GPIO_PIN_10

/* Motor B (right) */
#define MOTOR_B_PWMB_PORT       GPIOA
#define MOTOR_B_PWMB_PIN        GPIO_PIN_11     /* TIM1_CH4 */
#define MOTOR_B_BIN1_PORT       GPIOB
#define MOTOR_B_BIN1_PIN        GPIO_PIN_12
#define MOTOR_B_BIN2_PORT       GPIOB
#define MOTOR_B_BIN2_PIN        GPIO_PIN_13

/* STBY */
#define MOTOR_STBY_PORT         GPIOC
#define MOTOR_STBY_PIN          GPIO_PIN_13

/* PWM timer: TIM1, frequency = 20kHz, ARR = 3599 (72MHz / 3600 = 20kHz) */
#define MOTOR_PWM_TIM           (&htim1)
#define MOTOR_PWM_PERIOD        3599
#define MOTOR_MAX_SPEED         3599

void Motor_Init(void);
void Motor_SetSpeed(int16_t left, int16_t right);
void Motor_Brake(void);
void Motor_Standby(uint8_t enable);

#endif /* __MOTOR_H */
