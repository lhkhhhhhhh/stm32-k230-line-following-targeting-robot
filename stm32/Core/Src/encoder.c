#include "encoder.h"

static TIM_HandleTypeDef htim2, htim4;
static int32_t enc_left_total  = 0;
static int32_t enc_right_total = 0;
static uint16_t enc_left_last  = 0;
static uint16_t enc_right_last = 0;

void Encoder_Init(void)
{
    GPIO_InitTypeDef g = {0};
    TIM_Encoder_InitTypeDef e = {0};

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    /* left: TIM4, PB6=CH1, PB7=CH2 */
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    g.Pin  = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOB, &g);

    __HAL_RCC_TIM4_CLK_ENABLE();
    htim4.Instance = TIM4;
    htim4.Init.Prescaler         = 0;
    htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim4.Init.Period            = 0xFFFF;
    htim4.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    e.EncoderMode   = TIM_ENCODERMODE_TI12;
    e.IC1Polarity   = TIM_ICPOLARITY_RISING;
    e.IC1Selection  = TIM_ICSELECTION_DIRECTTI;
    e.IC1Prescaler  = TIM_ICPSC_DIV2;
    e.IC1Filter     = 0;
    e.IC2Polarity   = TIM_ICPOLARITY_RISING;
    e.IC2Selection  = TIM_ICSELECTION_DIRECTTI;
    e.IC2Prescaler  = TIM_ICPSC_DIV2;
    e.IC2Filter     = 0;

    HAL_TIM_Encoder_Init(&htim4, &e);
    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(&htim4, 0);

    /* right: TIM2, PA15=CH1(remap), PB3=CH2(remap) */
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_TIM2_REMAP)
               |  AFIO_MAPR_TIM2_REMAP_PARTIALREMAP1;

    g.Pin = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOB, &g);

    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = TIM2;
    htim2.Init.Prescaler         = 0;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 0xFFFF;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Encoder_Init(&htim2, &e);
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}

/* read as uint16_t: forward motion = monotonically increasing 0..65535 */
int32_t Encoder_GetLeft(void)  { return enc_left_total; }
int32_t Encoder_GetRight(void) { return enc_right_total; }

void Encoder_Update(void)
{
    uint16_t l = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);
    uint16_t r = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
    enc_left_total  += (int16_t)(l - enc_left_last);
    enc_right_total += (int16_t)(r - enc_right_last);
    enc_left_last  = l;
    enc_right_last = r;
}

void Encoder_Reset(void)
{
    __HAL_TIM_SET_COUNTER(&htim4, 0);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    enc_left_total  = 0;
    enc_right_total = 0;
    enc_left_last   = 0;
    enc_right_last  = 0;
}

void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef *htim)
{
    if      (htim->Instance == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
    else if (htim->Instance == TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
}