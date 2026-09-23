# September 8 — Modular Load Control Validation

## Objective

Verify Load_Init, Load_On, Load_Off, Load_Toggle, and Load_IsEnabled on the existing NUCLEO-F446RE / PB10 / 2N7000 / external LED circuit.

## Change

- Function declarations: [control.h](../../firmware/stm32_controller/Core/Inc/control.h)
- GPIO operations and private commanded-state record: [control.c](../../firmware/stm32_controller/Core/Src/control.c)
- Initialization and final blink loop: [main.c](../../firmware/stm32_controller/Core/Src/main.c). The temporary validation sequence was removed after testing; its procedure is preserved in the [walkthrough](../load_control_walkthrough.md).
- Circuit and previous measurements: [verified bring-up](2026-09-04_bringup.md)

## Build and Debug Setup

- Build configuration: Debug.
- Build result: supplied CubeIDE screenshot shows **0 errors, 0 warnings** for an incremental build.
- Build evidence: [CubeIDE screenshot](../images_videos/2026-09-08_load_control_build.png).
- Programming/debug connection actually used: STM32CubeIDE with the Nucleo's onboard ST-Link; the supplied photos show the ST-LINK GDB server debug session.
- Flash/debug launch result: PASS. The photos show `Download verified successfully` and an active debug session at the test checkpoints.
- ST-Link halted in main: PASS. The photos show execution suspended at the marked delay lines with `observed` visible in Variables.

## Function Checks

At each marked HAL_Delay line, execution was paused, `observed` was recorded from the Variables view, and the external LED was independently observed. The results below were recorded by Levi. At a breakpoint, the highlighted line has not yet executed. `observed` records the command, not measured current or light output.

Line numbers below refer to the temporary validation sequence shown in the photos, before it was removed from the final firmware.

| Stop | Action completed | Delay line | Expected observed | Actual observed | Expected LED | Actual LED | Pass/Fail |
|---|---|---:|---|---|---|---|---|
| A | Load_Init | 93 | false / 0 | false | OFF | OFF | pass |
| B | Load_On | 97 | true / 1 | true | ON | ON | pass |
| C | Load_Off | 101 | false / 0 | false | OFF | OFF | pass |
| D | Load_Toggle from OFF | 105 | true / 1 | true | ON | ON | pass |
| E | Load_Toggle from ON | 109 | false / 0 | false | OFF | OFF | pass |

Screenshots or additional observations:

- [A — Load_Init: observed = false, external LED OFF](../images_videos/init_false_off.jpg)
- [B — Load_On: observed = true, external LED ON](../images_videos/on_true_on.jpg)
- [C — Load_Off: observed = false, external LED OFF](../images_videos/off_false_off.jpg)
- [D — Load_Toggle from OFF: observed = true, external LED ON](../images_videos/toggle_true_on.jpg)
- [E — Load_Toggle from ON: observed = false, external LED OFF](../images_videos/toggle_false_off.jpg)

Each photo includes both the CubeIDE checkpoint/Variables view and the physical breadboard. The LED result refers to the external breadboard LED, not the Nucleo's onboard indicator LEDs. JPG copies were made for viewing in the repository; the original HEIC files were retained.

The OFF observation at A checks the state at that stop; it does not measure brief power-up glitches. Breakpoint pauses change elapsed timing, so this sequence is not a timing-accuracy test.

## Final Firmware Check

Levi confirmed on September 8 that the final firmware check was complete and the circuit behaved as it did before the temporary verification code was added.

- Temporary startup test replaced by one Load_Init call: COMPLETE; also confirmed in the saved source.
- Breakpoints removed/disabled: COMPLETE, per Levi's final-check confirmation.
- Rebuilt and flashed final firmware: COMPLETE, per Levi's final-check confirmation.
- External LED repeats approximately 1 second ON / 1 second OFF while running freely: PASS, per Levi's observation. The saved loop calls Load_On and Load_Off with HAL_Delay(1000) between them.

## Problems and Fixes

### Header organization

- Review finding: an early version of control.h defined `static bool load_enabled = false;`, while control.c defined it again. The header also lacked `<stdbool.h>` and an include guard.
- Cause: the header was being used to hold private state as well as the public function declarations. Including it in control.c introduced a duplicate initialized definition; the boolean type also needed its standard header.
- Fix: keep the private variable only in control.c, add `<stdbool.h>` to control.h, and wrap the declarations in `#ifndef CONTROL_H` / `#define CONTROL_H` / `#endif`.
- Verification: the saved header/source have this structure, and the supplied build screenshot reports zero errors and zero warnings.

### Debugger operation

- Difficulty: after building the Debug configuration with the hammer button, it was unclear how to start a hardware debug session, resume to breakpoints, and find the Variables view.
- Clarification: Build compiles the firmware; Debug starts the ST-Link session; Resume runs the board to the next breakpoint. Variables shows `observed` when execution is paused in main after its assignment.
- Verification: the five checkpoint photos show a working debug session with the expected variable values and physical LED states.

No new wiring or load-switching fault was reported during this lab. The USB connection issue belongs to the earlier bring-up lab and was not reported as recurring here.

## Conclusion

The GPIO load-control code was moved into a dedicated module and validated on the NUCLEO-F446RE with the existing PB10 / 2N7000 / external LED circuit. All five checkpoints passed: initialization commanded OFF, explicit ON/OFF commands produced the expected LED states, both toggle directions worked, and Load_IsEnabled returned the corresponding commanded state.

After the temporary verification sequence was removed, the final firmware resumed the expected approximately one-second ON / one-second OFF behavior. The refactor therefore preserved the demonstrated hardware behavior while separating the application's timing from the GPIO operations and private state record. The module provides a common interface for later PWM, UART control, and fault handling.

- Hardware/function validation: complete; no remaining failure reported for the checks above.
- Evidence: recorded results, five checkpoint photos, a successful build screenshot, and final-behavior confirmation.
- Final firmware/evidence commit: done.
