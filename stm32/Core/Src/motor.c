#include "motor.h"

static TIM_HandleTypeDef htim1;

void Motor_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_OC_InitTypeDef oc = {0};

    /* Enable clocks */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* GPIO: PWMA(PA8, AF), PWMB(PA11, AF) */
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = MOTOR_A_PWMA_PIN;
    HAL_GPIO_Init(MOTOR_A_PWMA_PORT, &gpio);

    gpio.Pin = MOTOR_B_PWMB_PIN;
    HAL_GPIO_Init(MOTOR_B_PWMB_PORT, &gpio);

    /* GPIO: AIN1(PA9), AIN2(PA10), BIN1(PB12), BIN2(PB13), STBY(PC13) - outputs */
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = MOTOR_A_AIN1_PIN | MOTOR_A_AIN2_PIN;
    HAL_GPIO_Init(MOTOR_A_AIN1_PORT, &gpio);

    gpio.Pin = MOTOR_B_BIN1_PIN | MOTOR_B_BIN2_PIN;
    HAL_GPIO_Init(MOTOR_B_BIN1_PORT, &gpio);

    gpio.Pin = MOTOR_STBY_PIN;
    HAL_GPIO_Init(MOTOR_STBY_PORT, &gpio);

    /* Initial state: all direction pins low, STBY high */
    HAL_GPIO_WritePin(MOTOR_A_AIN1_PORT, MOTOR_A_AIN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_A_AIN2_PORT, MOTOR_A_AIN2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_B_BIN1_PORT, MOTOR_B_BIN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_B_BIN2_PORT, MOTOR_B_BIN2_PIN, GPIO_PIN_RESET);
    Motor_Standby(1);

    /* TIM1: 72MHz clock, 20kHz PWM (PSC=0, ARR=3599) */
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 0;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = MOTOR_PWM_PERIOD;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim1);

    /* CH1 (PA8): left motor speed */
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1);

    /* CH4 (PA11): right motor speed */
    HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_4);

    /* Start PWM */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
}

/* left, right: -MOTOR_MAX_SPEED .. +MOTOR_MAX_SPEED. Positive = forward. */
void Motor_SetSpeed(int16_t left, int16_t right)
{
    /* Left motor (A) */
    if (left > 0) {
        HAL_GPIO_WritePin(MOTOR_A_AIN1_PORT, MOTOR_A_AIN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_A_AIN2_PORT, MOTOR_A_AIN2_PIN, GPIO_PIN_RESET);
    } else if (left < 0) {
        HAL_GPIO_WritePin(MOTOR_A_AIN1_PORT, MOTOR_A_AIN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_A_AIN2_PORT, MOTOR_A_AIN2_PIN, GPIO_PIN_SET);
        left = -left;
    } else {
        HAL_GPIO_WritePin(MOTOR_A_AIN1_PORT, MOTOR_A_AIN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_A_AIN2_PORT, MOTOR_A_AIN2_PIN, GPIO_PIN_RESET);
    }

    if (left > MOTOR_MAX_SPEED) left = MOTOR_MAX_SPEED;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)left);

    /* Right motor (B) */
    if (right > 0) {
        HAL_GPIO_WritePin(MOTOR_B_BIN1_PORT, MOTOR_B_BIN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_B_BIN2_PORT, MOTOR_B_BIN2_PIN, GPIO_PIN_RESET);
    } else if (right < 0) {
        HAL_GPIO_WritePin(MOTOR_B_BIN1_PORT, MOTOR_B_BIN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_B_BIN2_PORT, MOTOR_B_BIN2_PIN, GPIO_PIN_SET);
        right = -right;
    } else {
        HAL_GPIO_WritePin(MOTOR_B_BIN1_PORT, MOTOR_B_BIN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_B_BIN2_PORT, MOTOR_B_BIN2_PIN, GPIO_PIN_RESET);
    }

    if (right > MOTOR_MAX_SPEED) right = MOTOR_MAX_SPEED;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, (uint32_t)right);
}

void Motor_Brake(void)
{
    /* Short brake: both direction pins high */
    HAL_GPIO_WritePin(MOTOR_A_AIN1_PORT, MOTOR_A_AIN1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_A_AIN2_PORT, MOTOR_A_AIN2_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_B_BIN1_PORT, MOTOR_B_BIN1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_B_BIN2_PORT, MOTOR_B_BIN2_PIN, GPIO_PIN_SET);
}

void Motor_Standby(uint8_t enable)
{
    HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN,
        enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM1) {
        __HAL_RCC_TIM1_CLK_ENABLE();
    }
}
