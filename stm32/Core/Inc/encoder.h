#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f1xx_hal.h"

/* Left encoder: TIM4_CH1/CH2 on PB6/PB7. */
#define ENC_L_A_PORT    GPIOB
#define ENC_L_A_PIN     GPIO_PIN_6
#define ENC_L_B_PORT    GPIOB
#define ENC_L_B_PIN     GPIO_PIN_7

/* Right encoder: TIM2_CH1/CH2 on PA15/PB3 (partial remap). */
#define ENC_R_A_PORT    GPIOA
#define ENC_R_A_PIN     GPIO_PIN_15
#define ENC_R_B_PORT    GPIOB
#define ENC_R_B_PIN     GPIO_PIN_3

void Encoder_Init(void);
void Encoder_Update(void);
int32_t Encoder_GetLeft(void);
int32_t Encoder_GetRight(void);
void Encoder_Reset(void);

#endif
