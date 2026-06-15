#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f1xx_hal.h"

/* Left encoder: E2A->PB3, E2B->PB4 */
#define ENC_L_A_PORT    GPIOB
#define ENC_L_A_PIN     GPIO_PIN_3
#define ENC_L_B_PORT    GPIOB
#define ENC_L_B_PIN     GPIO_PIN_4

/* Right encoder: E1A->PA12, E1B->PA15 */
#define ENC_R_A_PORT    GPIOA
#define ENC_R_A_PIN     GPIO_PIN_12
#define ENC_R_B_PORT    GPIOA
#define ENC_R_B_PIN     GPIO_PIN_15

void Encoder_Init(void);
void Encoder_Update(void);
int32_t Encoder_GetLeft(void);
int32_t Encoder_GetRight(void);
void Encoder_Reset(void);

#endif
