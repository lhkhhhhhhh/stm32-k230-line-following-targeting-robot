#include "stm32f1xx_hal.h"

/* HAL_Delay() and HAL_GetTick() depend on this interrupt handler. */
void SysTick_Handler(void)
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}
