#include "main.h"
#include "line_sensor.h"
#include "motor.h"
#include "encoder.h"

#define PID_KP           200.0f
#define PID_KI             1.0f
#define PID_KD            25.0f
#define PID_I_LIMIT      500.0f

#define BASE_SPEED         600
#define MAX_REVERSE_SPEED  250
#define PIVOT_SPEED        600

#define K230_PORT   GPIOB
#define K230_PIN    GPIO_PIN_15

#define STOP_COUNT          4
#define STOP_DURATION_MS    8000

static const int32_t stop_enc_ticks[STOP_COUNT] = {
    15650,  /* Stop 1 */
    24870,  /* Stop 2 */
    31020,  /* Stop 3 */
    73000,  /* Stop 4 */
};

enum { STATE_RUNNING, STATE_STOPPED, STATE_DONE };

HAL_StatusTypeDef SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) return HAL_ERROR;
    clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) return HAL_ERROR;
    return HAL_OK;
}
void Error_Handler(void) { __disable_irq(); while(1); }

int main(void)
{
    HAL_Init();
    if (SystemClock_Config() != HAL_OK) Error_Handler();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LINE_SENSOR_PINS;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(LINE_SENSOR_PORT, &gpio);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = K230_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(K230_PORT, &gpio);
    HAL_GPIO_WritePin(K230_PORT, K230_PIN, GPIO_PIN_RESET);

    Encoder_Init();
    Encoder_Reset();
    Motor_Init();

    float integral = 0.0f;
    int16_t last_error = 0;
    uint32_t last_time = HAL_GetTick();
    int8_t last_dir = 0;
    int next_stop = 0;
    int state = STATE_RUNNING;
    uint32_t stop_start = 0;

    while (1)
    {
        uint32_t now = HAL_GetTick();
        int16_t pos = LineSensor_GetPosition();

        Encoder_Update();
        int32_t el = Encoder_GetLeft();
        int32_t er = Encoder_GetRight();
        int32_t enc_avg = (el + er) / 2;

        if (state == STATE_RUNNING) {
            if (enc_avg >= stop_enc_ticks[next_stop]) {
                state = STATE_STOPPED;
                stop_start = now;
                HAL_GPIO_WritePin(K230_PORT, K230_PIN, GPIO_PIN_SET);
            }
        }
        if (state == STATE_STOPPED) {
            Motor_SetSpeed(0, 0);
            if ((now - stop_start) >= STOP_DURATION_MS) {
                HAL_GPIO_WritePin(K230_PORT, K230_PIN, GPIO_PIN_RESET);
                next_stop++;
                if (next_stop >= STOP_COUNT) {
                    state = STATE_DONE;  /* back at start, stop */
                } else {
                    state = STATE_RUNNING;
                }
                integral = 0.0f;
            }
            HAL_Delay(5);
            continue;
        }
        if (state == STATE_DONE) {
            Motor_SetSpeed(0, 0);
            HAL_Delay(10);
            continue;
        }

        if (pos == 128) {
            integral = 0.0f;
            if      (last_dir > 0) Motor_SetSpeed( PIVOT_SPEED, -PIVOT_SPEED);
            else if (last_dir < 0) Motor_SetSpeed(-PIVOT_SPEED,  PIVOT_SPEED);
            else                   Motor_SetSpeed( PIVOT_SPEED,  PIVOT_SPEED);
            HAL_Delay(5);
            continue;
        }

        float dt = (float)(now - last_time) / 1000.0f;
        if (dt <= 0.0f) dt = 0.005f;
        last_time = now;

        float error = (float)pos;
        if (error > 1.0f) last_dir = 1;
        else if (error < -1.0f) last_dir = -1;

        integral += error * dt;
        if (integral > PID_I_LIMIT) integral = PID_I_LIMIT;
        if (integral < -PID_I_LIMIT) integral = -PID_I_LIMIT;

        float derivative = (float)(error - last_error) / dt;
        last_error = (int16_t)error;

        float correction = PID_KP*error + PID_KI*integral + PID_KD*derivative;

        /* Clamp in floating point before narrowing to int16_t. A derivative
           spike can exceed the integer range when the line position jumps. */
        float left_cmd = BASE_SPEED + correction;
        float right_cmd = BASE_SPEED - correction;

        if (left_cmd > MOTOR_MAX_SPEED) left_cmd = MOTOR_MAX_SPEED;
        if (left_cmd < -MAX_REVERSE_SPEED) left_cmd = -MAX_REVERSE_SPEED;
        if (right_cmd > MOTOR_MAX_SPEED) right_cmd = MOTOR_MAX_SPEED;
        if (right_cmd < -MAX_REVERSE_SPEED) right_cmd = -MAX_REVERSE_SPEED;

        int16_t left = (int16_t)left_cmd;
        int16_t right = (int16_t)right_cmd;

        Motor_SetSpeed(left, right);
        HAL_Delay(5);
    }
}
