# Guided PWM session: from generated timer to measured LED control

Prepared September 11, 2026, from the files currently saved in this repository.

**Start here:** complete one numbered step at a time. Each step tells you what to do, what you should see, and what to check if it differs. You do not need to plan the next feature while doing this one.

This guide contains the code for you to put into CubeIDE. Creating this guide has not changed your active firmware or flashed the board. The accompanying [PWM lab worksheet](lab_notes/pwm_validation.md) starts with hardware results pending.

The fixed-duty and final-demo examples were compiled and linked in an isolated temporary copy against this project's STM32 headers, drivers, startup code, and linker script on September 11. Both completed with no compiler or linker warnings. This checks the examples; you will still build your saved edits and validate them on the board.

## Your starting point is real progress

Your [bring-up record](lab_notes/2026-09-04_bringup.md) documents the external LED switching, 9.38 mA calculated from actual resistor measurements, and the gate pulldown keeping the LED OFF when control was disconnected. Your [September 8 record](lab_notes/2026-09-08_load_control.md) documents all five load-control function checks passing, with photos and a build screenshot.

You have already connected firmware to a physical circuit, measured its behavior, and reorganized working code without changing the observed result. Those are concrete engineering accomplishments you can explain using evidence.

The current files also show that you completed code generation for TIM2. The next piece is connecting your load functions to that timer. Asking where CubeMX hides a setting does not undo any of your progress. The earlier instructions left some steps for you to infer; this guide supplies those steps.

## Today's finish line and time budget

**Goal:** use the existing PB10 → 220 Ω → 2N7000 → external LED circuit to demonstrate and record 0%, 25%, 50%, 75%, and 100% duty settings, then restore a simple repeating demo.

Allow about 3–4 hours, including troubleshooting. The times below are planning allowances, not a speed test. If a checkpoint takes longer, record the exact point you reached and resume there.

| Steps | Allow | What you will have established |
|---|---:|---|
| 1–2 | 20 minutes | Configuration confirmed and timer numbers understood |
| 3–5 | 45–60 minutes | Load functions and a fixed-duty test entered |
| 6–7 | 30–45 minutes | Build/flash and physical OFF/50% checks |
| 8 | 45–60 minutes | Five settings measured and command checks recorded |
| 9–10 | 30–45 minutes | Repeating demo, evidence, and focused commit |

**Distraction ban:** keep the present LED circuit. UART, Python, sensors, fan control, Altium, and a broad C course wait until this checkpoint is complete. The roadmap's old dates are not instructions to skip unfinished dependencies.

## 1. Confirm what generation already accomplished

**This is a read-only check. Do not paste code or add lines in this step.** Rechecked September 15: your saved main.c and stm32f4xx_hal_msp.c already match all the settings below. **C0 is complete as a source check.** You can read through this step to recognize what CubeMX generated, then continue to step 2. No regeneration is needed for the current saved configuration.

1. Open your existing `stm32_controller` project in STM32CubeIDE. Do not create a new project.
2. In Project Explorer, expand `stm32_controller → Core → Src` and open [main.c](../firmware/stm32_controller/Core/Src/main.c).
3. Use **Edit → Find/Replace** to find `MX_TIM2_Init`. You should see its call after `MX_GPIO_Init()`, and its function definition farther down.
4. In that function, locate these existing lines. They need not be adjacent. Read and compare them; do not paste this block into the file:

```c
htim2.Init.Prescaler = 83;
htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
htim2.Init.Period = 999;
sConfigOC.OCMode = TIM_OCMODE_PWM1;
sConfigOC.Pulse = 0;
sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
```

5. In Project Explorer, open **Core → Src → [stm32f4xx_hal_msp.c](../firmware/stm32_controller/Core/Src/stm32f4xx_hal_msp.c)**. Find the body of `HAL_TIM_MspPostInit`, then the `if(htim->Instance==TIM2)` block inside it. That block is what “TIM2 branch” means. It already contains these lines; this is a viewing reference, not code to add:

```c
GPIO_InitStruct.Pin = GPIO_PIN_10;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
```

`GPIOB` plus `GPIO_PIN_10` selects PB10. `GPIO_MODE_AF_PP` lets a peripheral drive the pin both HIGH and LOW. `GPIO_AF1_TIM2` selects the pin's alternate-function connection to TIM2. `GPIO_NOPULL` disables the MCU's internal pull resistors; your external 10 kΩ pulldown stays in the circuit. `GPIO_SPEED_FREQ_LOW` controls the output edge-speed setting, not the 1 kHz PWM repetition rate. The final call applies these pin settings.

**Checkpoint C0 — configuration:** those entries match, and `MX_TIM2_Init()` runs before `Load_Init()`. This checkpoint was confirmed in the saved source; that does not yet prove a physical PWM output.

If a value differs, correct the [.ioc](../firmware/stm32_controller/stm32_controller.ioc) in CubeMX, then generate again. The UI path is **Pinout & Configuration → Timers → TIM2**. Mode: **Internal Clock** and **Channel3 → PWM Generation CH3**. Under **Configuration → Parameter Settings**, set Prescaler 83 and Counter Period 999. Scroll down to **PWM Generation Channel 3** for **Mode = PWM mode 1**, **Pulse (32 bits value) = 0**, and **CH Polarity = High**. Confirm PB10 is the selected channel pin.

When regenerating, save your code first and leave **Project Manager → Code Generator → Keep User Code when re-generating** checked. Keep manual changes in generated files inside the existing `USER CODE` markers. CubeMX's preservation option is shown in [ST's CubeMX workshop](https://www.st.com/content/dam/AME/2019/STM32G0-q1-webinar-workshops/STM32G0_Workshop_Presentation.pdf).

**If you already match C0, go straight to step 2. You do not need to repeat generation.**

## 2. Understand just enough to know what you are building

First separate the hardware names from the timing values: **TIM2** means timer unit number 2, and **CH3** means its output channel number 3. Those numbers identify hardware; they do not mean 2 Hz or 3 Hz. The timing values we calculate with are the timer input clock, prescaler, period, and compare value below.

PWM means repeated ON/OFF pulses. The pin still switches between approximately 0 V and 3.3 V; a 50% command makes it HIGH for half of each cycle. The LED's apparent brightness changes because its average current changes. Your eye does not judge brightness linearly.

There are three numbers to connect:

| Name | Meaning here |
|---|---|
| Prescaler, PSC = 83 | Divide the 84 MHz timer input by 84, giving one counter tick per microsecond |
| Auto-reload, ARR = 999 | Count from 0 through 999: 1,000 ticks, so one cycle takes 1 ms |
| Compare, CCR3 | Set how many ticks in each cycle channel 3 is HIGH |

Thus `84,000,000 / (83 + 1) / (999 + 1) = 1,000 Hz`. The 84 MHz timer input is recorded in your [.ioc](../firmware/stm32_controller/stm32_controller.ioc); its APB1 bus clock is 42 MHz, a different value. The timer performs the repeated switching in hardware. A delay in the application can give you time to observe a setting while the timer continues running. [ST timer guide](https://www.st.com/resource/en/application_note/dm00236305-pwm-generation-using-stm32-general-purpose-timers-stmicroelectronics.pdf)

| Requested duty | CCR3 for this configuration | HIGH time in each 1 ms cycle |
|---:|---:|---:|
| 0% | 0 | 0 µs |
| 25% | 250 | 250 µs |
| 50% | 500 | 500 µs |
| 75% | 750 | 750 µs |
| 100% | 1,000 | Continuously HIGH |

PWM mode 1 is active while the counter is less than the compare value. Since the counter only reaches 999, a compare value of 1,000 gives continuous HIGH. This is why the formula uses `ARR + 1`, including for 100%. [ST's PWM explanation](https://github.com/STMicroelectronics/STM32CubeC5/blob/main/examples/hal/tim/pwm_output/README.md)

**Understanding check:** at 50%, CCR3 should be 500 and the pin should be HIGH for about 500 µs per cycle. If that sentence makes sense, you know enough to implement the next step. You do not need to memorize the timer chapter.

## 3. Update the public function declarations

Open `Core → Inc →` [control.h](../firmware/stm32_controller/Core/Inc/control.h). Replace this small file's entire contents with:

```c
#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>
#include <stdint.h>

void Load_Init(void);
void Load_On(void);
void Load_Off(void);
void Load_Toggle(void);
bool Load_IsEnabled(void);

// Clamp commands below 0 to 0, and above 100 to 100.
void Load_SetPWM(int32_t percent);
uint8_t Load_GetPWM(void);

#endif
```

Save. A declaration tells the compiler what a function accepts and returns. `int32_t` is a signed 32-bit integer, so this interface can receive a negative command before deciding to clamp it to zero. `uint8_t` is an unsigned 8-bit integer, large enough to return a percentage from 0 through 100. `<stdint.h>` supplies those types.

**Checkpoint C1 — header:** the file has one include guard and seven function declarations. It contains no function bodies or private state variable. Wait until step 6 to build; the changes are not all in place yet.

## 4. Give the existing load functions their PWM implementation

Open `Core → Src →` [control.c](../firmware/stm32_controller/Core/Src/control.c). Replace this file's entire contents with the following. This is your own module, so it does not need CubeMX `USER CODE` markers.

```c
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
```

Save. Read these five points alongside the code:

1. `extern` refers to the existing `htim2` in main.c. Do not add a second definition. Your current project keeps timer setup in main.c, so this example does not require a `tim.h` file.
2. `HAL_TIM_PWM_Start` enables the configured channel and timer. Initialization alone does not start the output. `&htim2` passes the address of the timer handle so HAL can use it. [ST HAL implementation](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/master/Src/stm32f4xx_hal_tim.c)
3. `Load_SetPWM` limits the input, computes the compare value, writes it, and remembers the requested percentage. The multiplication here is sized for the current 1,000-count configuration; do not generalize it to arbitrary 32-bit timer periods without revisiting overflow.
4. ON means 100%; OFF means 0%. Toggle means any nonzero duty becomes 0%, and 0% becomes 100%. It does not remember and restore a previous dim setting.
5. The getters report the command. A disconnected LED can still have a software command of 50%; you need a separate physical check to establish what happened.

### Read the PWM calculation one line at a time

This is an explanation of the code above, not an additional block to paste. If your control.c already matches the complete example, leave it as it is while reading.

**What does `U` mean?** It is a C suffix meaning **unsigned integer**. `1U` is the number one written as an unsigned integer constant; `100U` is one hundred, and `0U` is zero. The numerical values do not change. Here, `1` has type `int`, while `1U` has type `unsigned int`. Unsigned integer types represent zero and positive integers, not negative numbers.

The suffix makes the constants' unsigned type explicit alongside the unsigned timer counts. It is not a unit, a variable, or a multiplication operation, and it does not mean microseconds. It also does not by itself specify a 32-bit width; `uint32_t` is the name that explicitly specifies an unsigned 32-bit integer. Keep the suffixes shown in the example, but there is no new setting to configure for them.

Start with the enclosing function:

```c
void Load_SetPWM(int32_t percent)
```

This defines a function named `Load_SetPWM`. `void` means it returns no value. It receives one signed 32-bit integer named `percent`. When you call `Load_SetPWM(25);`, `percent` starts with the value 25.

The preceding `if (percent < 0)` block replaces a negative input with zero. The `else if (percent > 100)` block replaces an input above 100 with 100. Inputs already between 0 and 100 stay unchanged. This ensures that the following conversions to unsigned types are applied to a value in the intended range.

**Line 1 — find how many counter positions make a whole PWM cycle:**

```c
uint32_t counts_per_cycle = __HAL_TIM_GET_AUTORELOAD(&htim2) + 1U;
```

- `uint32_t` declares an unsigned 32-bit integer variable.
- `counts_per_cycle` is the name we chose for that variable. It is local to this function call.
- `=` initializes it with the calculation on the right.
- `__HAL_TIM_GET_AUTORELOAD(...)` is an ST HAL macro: a named piece of C code that reads the timer's auto-reload register, **ARR**. A register is a hardware storage location used to configure or inspect a peripheral.
- `htim2` is the existing software handle identifying TIM2. `&htim2` means “the address of that handle,” letting the macro access the correct timer.
- Your ARR is **999**. The counter visits **0 through 999**, which gives **1,000 positions**. Adding `1U` accounts for counting zero as well.
- The result stored in `counts_per_cycle` is **1,000**. This line reads the configuration; it does not change the output.

**Line 2 — convert a percentage into the number of HIGH counts:**

```c
uint32_t compare = (counts_per_cycle * (uint32_t)percent) / 100U;
```

`compare` is another local unsigned 32-bit variable. `(uint32_t)percent` is a **cast**: it converts the already-limited percentage to an unsigned 32-bit value for this calculation. The parentheses here name a type conversion; they are not a function call. For an input of 25, the value is still 25 after the cast.

`*` multiplies and `/` divides. For `Load_SetPWM(25)`, the calculation is:

```text
compare = (1,000 × 25) / 100
        = 250
```

That means 250 HIGH counts out of 1,000 total counts. At your configured one-microsecond counter tick, that corresponds to about 250 microseconds HIGH per one-millisecond cycle.

These are integer calculations: division discards any fractional remainder. Multiplying before dividing preserves the useful result; calculating `25 / 100` first using integers would produce zero. With your current 1,000-count period, each whole percentage is exactly 10 counts.

**Line 3 — send the compare value to the timer hardware:**

```c
__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, compare);
```

This HAL macro writes a timer compare register. Its three arguments mean:

| Argument | Meaning |
|---|---|
| `&htim2` | Use TIM2 |
| `TIM_CHANNEL_3` | Use its third output channel, connected to PB10 |
| `compare` | Write the value just calculated, such as 250 |

For this channel, the destination register is **CCR3**. In the configured up-counting PWM mode 1 with active-HIGH polarity, the output is HIGH while the counter is less than CCR3. With CCR3 = 250, counts 0–249 are HIGH and counts 250–999 are LOW. The timer repeats that pattern without the application manually switching each pulse.

The local HAL definitions confirm that the first macro reads ARR and this macro selects CCR3 for channel 3: [timer HAL header](../firmware/stm32_controller/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_tim.h). This configuration buffers compare changes until a timer update event, so allow the running timer to reach its next cycle before interpreting the electrical result.

**Line 4 — remember the percentage the application requested:**

```c
load_pwm_percent = (uint8_t)percent;
```

`load_pwm_percent` is the private `static` variable declared near the top of control.c. It retains its value between function calls. `(uint8_t)` converts the clamped percentage to an unsigned 8-bit integer, which can hold 0–255. Your allowed 0–100 range fits without losing information.

For a 25% command, this line stores **25**, not the compare value 250. `Load_GetPWM()` later returns that percentage, and `Load_IsEnabled()` checks whether it is greater than zero. This line changes the software record; the preceding macro writes the hardware register. The software record does not measure whether the LED actually lit.

**The closing `}`** ends the function body. Execution returns to the code that called `Load_SetPWM`.

**Understanding checkpoint:** calling `Load_SetPWM(50)` should produce `counts_per_cycle = 1000`, `compare = 500`, a CCR3 command of 500, and `load_pwm_percent = 50`. Recognizing why the timer receives 500 while the software remembers 50 is the key idea here. You do not need to memorize the macro names to understand that conversion.

**Checkpoint C2 — module:** there are no `HAL_GPIO_WritePin` calls or `LOAD_CTRL_Pin` / `LOAD_CTRL_GPIO_Port` references left in control.c. Regeneration removed those old GPIO labels from main.h; replacing their use with timer control is the intended next step. Do not recreate the labels to silence an error.

## 5. Set up one steady test point

Open [main.c](../firmware/stm32_controller/Core/Src/main.c). Make only these two edits inside existing `USER CODE` regions. Keep the existing `#include "control.h"`.

**Edit A:** replace the contents between `USER CODE BEGIN 2` and `USER CODE END 2` with the body shown here. Keep one copy of each marker:

```c
  /* USER CODE BEGIN 2 */
  Load_Init();
  HAL_Delay(2000);  // Give yourself two seconds to observe the OFF state.

  Load_SetPWM(0);   // TEST SETTING: change only this number between tests.

  volatile uint8_t observed_pwm = Load_GetPWM();
  volatile bool observed_enabled = Load_IsEnabled();
  HAL_Delay(1000);  // CHECK: optional breakpoint here, after both assignments.
  (void)observed_pwm;
  (void)observed_enabled;
  /* USER CODE END 2 */
```

The generated calls `MX_GPIO_Init();` and `MX_TIM2_Init();` must remain immediately above this region. `volatile` keeps these temporary observation variables available for the debugger. The `(void)` lines mark them intentionally used; they do not change the output.

**Edit B:** remove the old `Load_On`, `Load_Off`, and their one-second delays from the while loop. The whole loop should now look like this:

```c
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HAL_Delay(1000);  // The hardware timer maintains the selected PWM.
  }
  /* USER CODE END 3 */
```

Replace the existing loop; do not paste a second while loop inside it. Save.

**Checkpoint C3 — application:** the program initializes the load, waits two seconds at OFF, sets one duty value, and then leaves that value alone. If you leave the old blink calls in place, they will overwrite your selected duty repeatedly. Removing them is part of the test setup.

## 6. Build and start the board

1. Save all edited files. Right-click the project in Project Explorer → **Refresh**.
2. Right-click the project → **Build Configurations → Set Active → Debug**.
3. Select the project, then **Project → Build Project**. Read the latest build output in Console; record errors and warnings in the [worksheet](lab_notes/pwm_validation.md).
4. Resolve any build errors before programming. Use the table below, starting with the first error rather than the total count.
5. Connect the Nucleo through the working USB hub. If Parallels has captured ST-Link, assign it to macOS or quit Parallels.
6. In main.c, double-click the editor's left margin beside `HAL_Delay(1000); // CHECK` in the startup code to set a breakpoint. In **Window → Show View → Other… → Debug → Breakpoints**, disable older breakpoints and leave this CHECK breakpoint enabled.
7. In **Run → Debug Configurations**, select the existing STM32 application configuration for `stm32_controller`, then **Debug**. Reuse the configuration that worked for your September 8 checks. Accept the Debug perspective if prompted. If execution stops at main, press **Resume** (green play/continue icon or **Run → Resume**) to reach CHECK.
8. At CHECK, open **Window → Show View → Other… → Debug → Variables**. Record `observed_pwm` and `observed_enabled` in the worksheet. For this first zero-duty test, expect `0` and `false`. The assignments above the highlighted delay line have executed; the highlighted delay has not.
9. Disable the CHECK breakpoint and press **Resume** before measuring voltage. The program must be running freely for electrical measurements. Re-enable CHECK before the next setting's debug launch so you can record the next command state.

| If you see… | Do this |
|---|---|
| `LOAD_CTRL_Pin` or `LOAD_CTRL_GPIO_Port` undeclared | Check that control.c was fully replaced and saved; its old GPIO implementation is still being compiled |
| `undefined reference` to `Load_SetPWM` or `Load_GetPWM` | Confirm definitions are in `Core/Src/control.c`, save, and Refresh; ensure control.c is not excluded from the Debug build |
| `htim2` undeclared | Restore the `extern TIM_HandleTypeDef htim2;` line in control.c |
| `multiple definition of htim2` | Keep the definition in generated main.c and only the `extern` declaration in control.c |
| `HAL_TIM...` types/functions missing | Confirm TIM2 remains enabled in the .ioc and regenerate; then Refresh and **Project → Clean…** for this project followed by Build Project |
| No ST-Link detected | Use the hub path and check Parallels ownership; this is a connection check, not a reason to rewrite PWM code |
| Debug session halted at main | Resume; the load initialization has not necessarily run yet |

**Checkpoint C4 — software delivery:** record the actual successful build and download result. A successful build proves the program can be compiled and linked. A verified download proves it was programmed. Neither alone proves the electrical output.

## 7. Get two clear physical wins: OFF, then 50%

Use the same already validated breadboard. Before powering it after handling, confirm the common STM32/external-supply ground, PB10/D6 control wire, existing 220 Ω gate resistor and 10 kΩ pulldown, LED polarity/series resistor, and unchanged MOSFET orientation against your [recorded circuit](lab_notes/2026-09-04_bringup.md). If rebuilding, use the exact manufacturer datasheet recorded there. Do not change the load to a motor or fan for this test.

**Test 0 — OFF:** with `Load_SetPWM(0)`, run freely and enable the external 5 V supply. The external breadboard LED should remain OFF. Watch through one reset: it should stay visibly OFF for this all-zero test. This is a visible observation, not a measurement of brief startup glitches.

Prepare the multimeter:

1. Black lead in **COM**, red lead in **V/Ω**; select **DC volts**, autorange or a range covering 3.3 V (often 20 V).
2. Put the black probe on the common ground.
3. Put the red probe on PB10/D6, on the MCU side of the 220 Ω resistor. Avoid bridging adjacent contacts. Keep this location for every row. Do not use the current jack for this voltage check.
4. Wait for a stable reading. Record the actual value in volts; expected at 0% is near zero.

**Checkpoint C5 — OFF:** LED visibly OFF and PB10 near 0 V. Record the results before changing anything. This establishes a useful output endpoint.

**Test 50 — your first intermediate setting:** in main.c, change only `Load_SetPWM(0)` in the startup test to `Load_SetPWM(50)`. Save, build, terminate the old debug session if one is active, re-enable CHECK, launch Debug again, and Resume to CHECK. Record `observed_pwm = 50` and `observed_enabled = true` if those are the values you actually see. Disable CHECK and Resume before measuring. Rebuilding without downloading does not update the board.

After the initial OFF interval, expect a steady-looking external LED and a PB10 DC average around 1.65 V, using a nominal HIGH level of 3.3 V. These are expectations, not reported measurements. It should not blink at one-second intervals; that would suggest the old loop or old firmware is still running.

**Checkpoint C6 — intermediate control:** record LED behavior and the stable DC reading. The expected interpretation is that the timer is delivering an intermediate duty command. Visual brightness and a DMM average cannot establish the pulse frequency or edge quality.

If the result differs, use this order: confirm execution is running → confirm the new build was flashed → confirm the old blink loop is gone → check `Load_Init()` starts channel 3 → check the compare command → check PB10-to-ground electrically → check gate and external load wiring. If PB10 is correct but the LED is not, focus on the path after PB10.

When C6 passes, give yourself credit for the specific result: you have made a software percentage affect a physical output. That is the capability later UART commands and Python tests will use.

## 8. Record all five settings without racing a changing output

Keep the steady test program. Complete the remaining fixed settings in this order: **100%, 25%, 75%, then return to 0%**. Together with step 7, this covers all five values. For every setting, change only the number in `Load_SetPWM(...)`, then save → build → re-enable CHECK → download/debug → Resume to CHECK → record command variables → disable CHECK and Resume → measure. Do not repeat completed 0%/50% measurement rows; returning to 0% at the end is a separate OFF recheck.

At 100%, record the actual PB10 HIGH voltage. Use that current measurement to calculate the expected averages for the intermediate rows, instead of treating the older 3.31 V bring-up measurement as today's result.

| Duty | CCR3 | Nominal DC average if HIGH = 3.3 V | Observation to look for |
|---:|---:|---:|---|
| 0% | 0 | 0.000 V | External LED OFF |
| 25% | 250 | 0.825 V | LED ON, dimmer than full duty |
| 50% | 500 | 1.650 V | Intermediate output |
| 75% | 750 | 2.475 V | Higher average than 50% |
| 100% | 1,000 | 3.300 V | LED continuously ON |

For a more useful comparison, calculate `expected average = measured V_LOW + duty/100 × (measured V_HIGH − measured V_LOW)`. Record the signed difference `measured average − expected average`. The worksheet separates nominal examples from your actual measurements.

Treat the DMM measurements as a functional average-voltage check: endpoints near LOW/HIGH and increasing intermediate readings broadly consistent with duty. Do not claim a calibrated accuracy specification without the meter's relevant performance information and predefined tolerance. If an intermediate average looks wrong, confirm probe location and running state, repeat it, and consult the meter's DC response specification before concluding the timer failed.

**Getter checkpoint:** the command observations recorded at CHECK during these same runs validate `Load_GetPWM()` and `Load_IsEnabled()`. Expect the requested percentage, with `false` for zero or `true` for a nonzero setting. No separate repeat of the five-point test is needed if those entries are already filled.

The timer's compare register uses preload in this HAL setup: a new value takes effect on an update event. A debugger halt may freeze the timer or let it run, depending on debug configuration. A reading while halted is not the normal-running PWM check. [Local HAL source](../firmware/stm32_controller/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c)

After the five main points, use the same startup line for these brief command checks. For the two toggle rows, replace that single line with the two calls shown. Keep the observation assignments after the calls. Build and download for each row; inspect the variables, Resume, then observe the LED. Record results in the worksheet.

| Startup command(s) | Expected observed_pwm | Expected observed_enabled | Expected settled LED |
|---|---:|---|---|
| `Load_SetPWM(-10);` | 0 | false | OFF |
| `Load_SetPWM(110);` | 100 | true | ON |
| `Load_On();` | 100 | true | ON |
| `Load_Off();` | 0 | false | OFF |
| `Load_SetPWM(50); Load_Toggle();` | 0 | false | OFF |
| `Load_Off(); Load_Toggle();` | 100 | true | ON |

If you have a scope, measure PB10-to-common-ground at 50% while running; a useful starting display is DC coupling, approximately 1 V/div, 200 µs/div, rising-edge trigger around 1.5 V. Record frequency, period, HIGH time, and duty. The configured expectations are approximately 1 kHz, 1 ms, 500 µs, and 50%. Keep scope validation marked **not performed** if you do not have the instrument.

**Checkpoint C7 — evidence:** five settings recorded, OFF rechecked, getter and wrapper/clamp results recorded, and instrument limitations stated. A result that differs is a debugging observation to investigate; it is not a judgment of your ability.

## 9. Restore a simple demonstration program

When the steady tests are complete, remove the temporary startup test and observation variables. Set the existing `USER CODE 2` region to:

```c
  /* USER CODE BEGIN 2 */
  Load_Init();
  HAL_Delay(2000);
  /* USER CODE END 2 */
```

Replace the existing while loop with this one. Preserve the generated markers and put one loop in main:

```c
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Load_Off();
    HAL_Delay(5000);
    Load_SetPWM(25);
    HAL_Delay(5000);
    Load_SetPWM(50);
    HAL_Delay(5000);
    Load_SetPWM(75);
    HAL_Delay(5000);
    Load_On();
    HAL_Delay(5000);
  }
  /* USER CODE END 3 */
```

Save, build, download, disable breakpoints, and Resume. Watch two full cycles. The LED should step through OFF → 25% → 50% → 75% → full ON, each held for about five seconds. Exact visual brightness ratios are not required. The delay selects how long you see each setting; TIM2 generates the fast pulses throughout each hold.

Save a short video showing one full cycle under `docs/images/`, using the actual capture date in its filename. Use the external LED as your visual reference; onboard LED2 is initialized but is not the load indicator in this test.

**Checkpoint C8 — final behavior:** the final program was built/downloaded and two cycles observed. Testing an earlier temporary program is not proof that this final revision was flashed.

## 10. Close the session with evidence you can point to

1. Finish the [PWM lab worksheet](lab_notes/pwm_validation.md): actual session date, board, instrument, configuration, build/download outcomes, tables, problems, and final demo result. Link the photo/video you actually saved.
2. In the [README](../README.md), mark PWM validated only to the extent supported by the results. With DMM-only evidence, say the timer was **configured for nominal 1 kHz** and five duties were checked using average voltage and LED observations; do not call 1 kHz a measured result.
3. Review the changed files. This milestone legitimately includes the .ioc, generated TIM setup and HAL timer driver files, your control module, and evidence. Leave unrelated local IDE preferences out of the commit unless deliberately needed.
4. Make a focused commit after the physical checks are recorded. A suitable message is `feat: validate timer PWM LED control`. If you want help with Git at that point, report “C8 complete, worksheet filled; review and commit the PWM milestone.”
5. Finish with three sentences in the lab note: “I changed…”, “I verified…”, and “The remaining uncertainty is…”. Use your actual results.

**DONE WHEN:** C0–C8 are supported by source checks or recorded observations as appropriate, actual measurements and limitations are saved, the final firmware behaves as recorded, and the milestone is committed. An unavailable scope does not block the DMM-based milestone, but waveform/frequency verification remains unperformed.

If time runs out, leave a useful handoff: `Stopped at C__; last build result __; board behavior __; next exact action __.` You can continue from that checkpoint without reconstructing the whole session.

## Why this step helps your confidence and your project

After bring-up you could say, “My MCU switches an external load.” After this checkpoint you will be able to add, “I choose its duty cycle using a hardware timer and compare the output with measurements.” The existing load functions become the interface that later serial commands can call.

Confidence can grow from repeatedly making a prediction, performing one action, and checking the result. This guide gives you a small, visible result at each stage: correct configuration, a successful build, a controlled OFF state, an intermediate output, and a recorded five-point experiment. You do not have to feel certain before beginning; you can use those results to decide what you know.

At a fair or interview, the useful story is your reasoning: why the timer divides down to 1 kHz, how duty becomes a compare value, what you measured, and what your instruments could not prove. That gives you specific work to discuss. It does not require pretending you wrote every line without help.

If you get stuck, include the checkpoint name, the exact first error or unexpected reading, and what you already checked. “C4: LOAD_CTRL_Pin undeclared in control.c” is enough to identify a concrete mismatch. We can resolve that step together without restarting the project.
