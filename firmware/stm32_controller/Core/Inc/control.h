#ifndef CONTROL_H  // Have we already processed this header?
#define CONTROL_H  // If not, mark it as processed.

#include <stdbool.h>  // Supplies bool, true, and false.
#include <stdint.h>

// Declare a function that takes no arguments and returns nothing.
void Load_Init(void);

// Declare the available control operations.
// Their actual instructions live in the source file.
void Load_On(void);
void Load_Off(void);
void Load_Toggle(void);

// Declare a function that takes no arguments and returns true/false.
bool Load_IsEnabled(void);

// Clamp commands below 0 to 0, and above 100 to 100.
void Load_SetPWM(int32_t percent);
uint8_t Load_GetPWM(void);

#endif  // End the guarded section.
