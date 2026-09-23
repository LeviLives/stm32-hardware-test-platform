#include "control.h"
#include "main.h"

// CubeMX defines this timer handle once, in main.c.
extern TIM_HandleTypeDef htim2;

// Last commanded percentage; this is not a sensor reading.
static uint8_t load_pwm_percent = 0U;

void Load_Init(void)
{
    // Call once, after MX_TIM2_Init(). Start with zero duty.
    Load_Off();

    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }
}

void Load_SetPWM(int32_t percent)
{
    if (percent < 0)
    {
        percent = 0;
    }
    else if (percent > 100)
    {
        percent = 100;
    }

    // This project uses ARR = 999, giving 1,000 counts per cycle.
    uint32_t counts_per_cycle = __HAL_TIM_GET_AUTORELOAD(&htim2) + 1U;
    uint32_t compare = (counts_per_cycle * (uint32_t)percent) / 100U;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, compare);
    load_pwm_percent = (uint8_t)percent;
}

uint8_t Load_GetPWM(void)
{
    return load_pwm_percent;
}

void Load_On(void)
{
    Load_SetPWM(100);
}

void Load_Off(void)
{
    Load_SetPWM(0);
}

void Load_Toggle(void)
{
    if (Load_IsEnabled())
    {
        Load_Off();
    }
    else
    {
        Load_On();
    }
}

bool Load_IsEnabled(void)
{
    return load_pwm_percent > 0U;
}
