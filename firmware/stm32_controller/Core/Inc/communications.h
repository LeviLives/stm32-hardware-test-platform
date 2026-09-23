#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

#include "stm32f4xx_hal.h"

// Returns nothing and accepts a pointer to the UART connection.
void Communications_Init(UART_HandleTypeDef *uart);

// Returns nothing and accepts no arguments.
void Communications_Process(void);

#endif
