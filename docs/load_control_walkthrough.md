# Load control: purpose, worksheet, and completed reference

This is a guided exercise for the already verified PB10 -> 2N7000 -> external LED circuit. The examples have not been built, flashed, or physically tested as part of writing this worksheet. Keep new hardware checks pending until you perform them.

## What this accomplishes

The visible result stays the same: the external LED turns ON for one second and OFF for one second. The change gives the project one place to implement each load operation. Later, a laptop command, an automated experiment, and fault shutdown can all request the same operation.

Your existing `HAL_GPIO_WritePin(...)` already is a function. `Load_Off()` adds project meaning: set the control output LOW and remember that the load was commanded OFF. This is a small organization improvement, not a new electrical capability or a speed improvement. It does not need a whole study day.

The private state is a software record of the last command, not a measurement of light or current. If the external supply is disconnected, a command can say ON while the LED is physically OFF.

## Why keep the record private?

Imagine another part of the program writes `load_enabled = false;` without changing PB10. The record says OFF while the pin can remain HIGH. Keeping the variable private encourages all callers to use `Load_Off()`, which performs both jobs together.

`static bool load_enabled = false;` outside the functions makes its name private to this source file. It remains changeable. `const` would prevent reassignment and would be wrong for a changing ON/OFF record.

This is protection against programming mistakes, not security. It cannot stop other code from independently writing PB10 through HAL. Our project convention is that application code requests output changes through the control module. Generated GPIO initialization still establishes the initial LOW output.

## What the files mean

- [main.c](../firmware/stm32_controller/Core/Src/main.c): chooses the sequence and timing.
- [control.h](../firmware/stm32_controller/Core/Inc/control.h): lists the available operations and their argument/return types.
- [control.c](../firmware/stm32_controller/Core/Src/control.c): contains the full function bodies and private state.

A header declaration such as `void Load_On(void);` has no body. The implementation `void Load_On(void) { ... }` supplies the actual instructions. In this layout, implementations belong in the source file. C permits some other arrangements, but they are unnecessary for this module.

`#include "control.h"` makes those declarations available to a caller. It does not run the functions. The build compiles the source files and links them into one firmware program. Calling `Load_On()` executes its body and then returns to the caller; source files are not separate running programs.

"Used outside this file" means, for example, a function defined in the control source being called from the main source. The five public functions allow this. A function marked `static` would have a name private to its own compilation unit (the source file plus included headers). Ordinary local variables declared inside a function already have local scope; they are not automatically shared with other files.

## 1. Check the baseline — 10 minutes

Open the existing `stm32_controller` project in STM32CubeIDE. Use the working USB hub connection. Build and flash the existing firmware and confirm the one-second external LED blink before changing the application.

The two control files already exist as empty files in the repository at the time this guide was written. Open those files in CubeIDE. If necessary, right-click the project and choose Refresh. A new project is unnecessary.

## 2. Complete the header — 5 minutes

Put this in the control header:

```c
#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>  // Supplies bool, true, and false.

// These are declarations: what other files are allowed to call.
// The bodies will be written once, in the control source file.
void Load_Init(void);       // Establish the OFF state after GPIO setup.
void Load_On(void);         // Request ON; repeated ON stays ON.
void Load_Off(void);        // Request OFF; repeated OFF stays OFF.
void Load_Toggle(void);     // Reverse the remembered command.
bool Load_IsEnabled(void);  // Read the command record without changing it.

#endif
```

The first two lines and final `#endif` prevent repeated processing of this header within a compilation unit. Do not include the source file or define the state variable in the header.

## 3. Guided source frame — 20–30 minutes

One function is worked through. The remaining comments specify exactly what to implement. This frame is intentionally unfinished; complete the TODOs before building or flashing. The next section contains the full reference if you prefer to follow it directly.

```c
#include "control.h"  // Our public declarations and bool type.
#include "main.h"     // Generated LOAD_CTRL pin names and HAL declarations.

// Private record of the last command. It is not an LED/current sensor.
// All the functions below share this one variable.
static bool load_enabled = false;

void Load_Init(void)
{
    // TODO: Call Load_Off().
    // main must first call the generated MX_GPIO_Init().
}

void Load_On(void)
{
    // Worked example, part 1: ask the STM32 to drive LOAD_CTRL HIGH.
    HAL_GPIO_WritePin(LOAD_CTRL_GPIO_Port, LOAD_CTRL_Pin, GPIO_PIN_SET);

    // Worked example, part 2: remember the ON command.
    // This line changes the record; the HAL call changes the output.
    load_enabled = true;
}

void Load_Off(void)
{
    // TODO: Use the same HAL call as Load_On, but GPIO_PIN_RESET.
    // TODO: Then assign false to load_enabled.
}

void Load_Toggle(void)
{
    // TODO: If load_enabled is true, call Load_Off().
    //       Otherwise, call Load_On().
    // Reuse those functions so the output and record change together.
}

bool Load_IsEnabled(void)
{
    // TODO: Return load_enabled. Do not write the pin here.
}
```

## Completed source reference

You can use this directly. Understanding which lines affect hardware and which affect the software record is the learning objective.

```c
#include "control.h"
#include "main.h"

// Private, mutable command record shared by this file's functions.
static bool load_enabled = false;

void Load_Init(void)
{
    // The generated code already configured PB10 as an output.
    // Establish OFF in both the output and our record.
    Load_Off();
}

void Load_On(void)
{
    // Drive the MOSFET control signal HIGH.
    HAL_GPIO_WritePin(LOAD_CTRL_GPIO_Port, LOAD_CTRL_Pin, GPIO_PIN_SET);

    // Record what we requested.
    load_enabled = true;
}

void Load_Off(void)
{
    // Drive the MOSFET control signal LOW.
    HAL_GPIO_WritePin(LOAD_CTRL_GPIO_Port, LOAD_CTRL_Pin, GPIO_PIN_RESET);

    // Record what we requested.
    load_enabled = false;
}

void Load_Toggle(void)
{
    // Reverse the last command through the same ON/OFF functions.
    if (load_enabled)
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
    // Answer the question without changing anything.
    return load_enabled;
}
```

Keep the five public functions non-static so the application can call them. Keep delays in the application: the control functions act immediately, and the caller chooses how long to wait.

## 4. Use the operations in the main source — 10 minutes

Make three edits in existing USER CODE regions. Preserve all generated markers and surrounding code.

Inside `USER CODE BEGIN Includes`, add:

```c
#include "control.h"
```

Inside `USER CODE BEGIN 2`, immediately after the existing generated `MX_GPIO_Init()` call, add:

```c
Load_Init();  // GPIO is ready; establish OFF.
```

Inside the existing while loop's `USER CODE BEGIN 3` region, replace the old manual HIGH/LOW writes and delays with:

```c
Load_On();       // Choose the action.
HAL_Delay(1000); // Choose how long to keep it.

Load_Off();
HAL_Delay(1000);
```

Do not paste another while loop inside the existing loop. Preserve its closing brace and the existing `USER CODE END 3`. The generated GPIO configuration and `.ioc` need no changes for this exercise.

## 5. Build and check — 20–30 minutes

Select Project -> Build Project. Resolve errors and new warnings, then flash through the working hub. Confirm the external LED still alternates one second ON and one second OFF.

To check all five functions, temporarily replace the `Load_Init()` line in USER CODE 2 with this sequence:

```c
Load_Init();
bool observed = Load_IsEnabled();
HAL_Delay(3000); // A: expected false, LED OFF.

Load_On();
observed = Load_IsEnabled();
HAL_Delay(3000); // B: expected true, LED ON.

Load_Off();
observed = Load_IsEnabled();
HAL_Delay(3000); // C: expected false, LED OFF.

Load_Toggle();
observed = Load_IsEnabled();
HAL_Delay(3000); // D: expected true, LED ON.

Load_Toggle();
observed = Load_IsEnabled();
HAL_Delay(3000); // E: expected false, LED OFF.

(void)observed;
```

Use the Debug build. Set breakpoints by double-clicking the editor's left margin on the five marked delay lines. Start the existing debug configuration and resume to each stop. In Variables, inspect `observed` and separately look at the external LED. Each operation and query has finished before its delay-line breakpoint. If the variable is unavailable, confirm that you are paused in main after its declaration in a Debug build; you can also step into the query function to inspect the private state.

After checking, restore the initialization region to just `Load_Init();`, remove breakpoints, rebuild, flash, and confirm the final one-second blink again. The initialization check establishes OFF at checkpoint A; it does not measure brief power-up glitches.

## 6. Record and finish — 10–15 minutes

Create a note dated for the actual session in [the lab-notes folder](lab_notes). Record build/flash outcomes, the observed five-function results, final blink behavior, and any debugging. Keep unperformed checks pending. Update the [README](../README.md) when modular control is verified, review the changes, and make a focused commit after validation.

DONE WHEN: the application uses the load functions; all five operations have been checked; the physical LED behavior remains correct; the actual observations and changes are documented and committed.

Distraction ban: stay with the existing LED circuit and these five operations. The next hardware capability is timer PWM. This small refactor should leave most of a 3–4-hour session available for understanding, debugging, and documenting rather than require hours of C study.
