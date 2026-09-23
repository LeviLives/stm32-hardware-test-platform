# Hardware PWM validation — September 22, 2026

Guide prepared September 11, 2026. **Validation performed September 22: five duty settings, command-state reporting, ON/OFF, both toggle directions, and percentage limits checked.**

The STM32 load-control module was changed from direct GPIO writes to TIM2 channel 3 PWM on PB10. Levi recorded DC averages of **0, 0.82, 1.64, 2.46, and 3.28 V** at **0/25/50/75/100%** commands, with the LED OFF at zero and lit at the other settings. The intermediate voltages match the proportions calculated from the measured endpoints at the recorded precision. All five command-state rows and all six wrapper/limit checks were recorded as matching their expectations.

**Completed scope:** fixed-duty functional validation, command checks, and repeating-demo confirmation (C0–C8). **Outside the completed scope:** oscilloscope measurements and startup-glitch characterization. The saved firmware contains the repeating demo, and Levi supplied a demo video linked below. Levi confirmed that the current saved firmware was flashed onto the STM32 and that the video contains two complete cycles (C8).

Procedure: [guided PWM session](../pwm_walkthrough.md).

## Setup

- Board: NUCLEO-F446RE / STM32F446RET6.
- Circuit: existing PB10/D6 → 220 Ω → 2N7000 gate, 10 kΩ pulldown, external LED and series resistor, common ground; [earlier validated circuit](2026-09-04_bringup.md).
- Circuit changes since bring-up: no separate change record supplied for this session; the linked bring-up note is the circuit reference.
- Firmware under test: [control.h](../../firmware/stm32_controller/Core/Inc/control.h), [control.c](../../firmware/stm32_controller/Core/Src/control.c), generated TIM2 configuration, and the fixed-duty startup test in [main.c](../../firmware/stm32_controller/Core/Src/main.c). The test command was changed between runs. After data collection, the saved closeout source was changed to a repeating 0/25/50/75/100% sequence with one second at each setting. The former `Load_SetPWM(110)` startup test is commented out; a commit ID will identify this snapshot once committed.
- Timer configuration in saved source: TIM2 channel 3, PB10 AF1, timer input 84 MHz, PSC 83, ARR 999, up counting, PWM mode 1, active HIGH, initial pulse 0.
- Configured frequency: nominal 1 kHz (calculated, not yet measured).
- Instrument: multimeter in DC-voltage mode, per Levi's report. Make/model and selected range were not recorded; quantitative meter uncertainty cannot be assigned.
- Oscilloscope: not available for this test; no scope measurements performed.
- Measurement location specified by the procedure: PB10 on the MCU side of the gate resistor to common ground. Levi supplied the readings in response to that procedure; no separate probe-placement capture was supplied.
- External supply voltage during this session: not recorded. The earlier bring-up supply measurement is not reused as a new measurement.
- Build result: PASS — the CubeIDE Console inspected during this September 22 session showed `Build Finished. 0 errors, 0 warnings.` The console was observed directly; a separate build screenshot has not been archived for this session.
- Download/debug result: PASS — [Levi's supplied debug-launch screenshot](../images_videos/2026-09-22_pwm_debug_launch.png) shows `File download complete` and execution paused at main.c:78. Levi subsequently confirmed execution reached CHECK with `observed_pwm = 0` and `observed_enabled = false`. This establishes the debug launch and command-state check, not electrical output behavior.
- Electrical readings were requested after disabling CHECK and resuming execution. The supplied screenshots document paused command checks, not the running state at the instant of a voltage reading; no separate capture of that state was supplied.
- Closeout build: the current saved Debug configuration was forcibly rebuilt with the installed STM32 toolchain on September 22; compilation and linking completed with **0 errors and 0 warnings**. This software check did not reflash the board or add a hardware result.

## Pre-test expectations

For this functional DMM check, expect approximately zero at 0%, approximately the GPIO HIGH level at 100%, and increasing intermediate averages consistent with the selected duty. Numeric accuracy certification is not part of this test. If applying a numerical pass tolerance, record it and its basis **before collecting results**.

- Optional predeclared DMM comparison tolerance and instrument basis: not specified; this is a functional comparison.
- Formula: expected average = measured LOW + (duty / 100) × (measured HIGH − measured LOW).
- Signed voltage difference = measured average − expected average.
- DMM averages do not verify pulse frequency, waveform shape, or brief startup behavior.

## Checkpoint record

| Checkpoint | Required evidence | Result / notes |
|---|---|---|
| C0 | Saved TIM2 configuration and PB10 routing match guide | PASS — source rechecked September 22: TIM2 CH3 / PB10 AF1, PSC 83, ARR 999, PWM1, active HIGH, initial pulse 0 |
| C1 | Header has seven public declarations | PASS — control.h inspected September 22; include guard, bool/integer headers, and seven declarations present |
| C2 | Load module uses timer compare values; no old GPIO labels | PASS — control.c inspected September 22; zero-duty initialization, PWM start, compare writes, range clamp, and command record present; no old LOAD_CTRL GPIO references |
| C3 | Fixed-duty test entered; old blink loop removed | PASS — fixed-duty runs produced the recorded results; TIM2 initializes before Load_Init. After collection, the saved loop was changed to the repeating demo described below |
| C4 | Actual successful build and download | PASS — build Console observed with 0 errors / 0 warnings; supplied screenshot shows download complete; Levi confirmed CHECK values after resuming |
| C5 | Zero-duty LED OFF and PB10 LOW | PASS — Levi reported external LED completely OFF and 0 V during the 0% test, September 22 |
| C6 | 50% output observation and DC voltage | PASS — functional observation: Levi reported external LED lit and 1.64 V DC average at 50%, September 22; waveform/frequency not measured |
| C7 | Five points and command checks recorded | PASS — five DC/LED observations, all five command-state checks, and six wrapper/limit checks recorded as expected; separate post-sweep voltage recheck not recorded |
| C8 | Final demo flashed and two cycles observed | PASS — Levi confirmed the current saved firmware was flashed and reviewed two complete cycles in the PWM demo video retained in the private development archive; confirmation is user-reported |

## Five-point measurements

Nominal voltages are examples using a 3.3 V HIGH level. The endpoint-based calculations use this session's measured LOW = 0 V and HIGH = 3.28 V. Actual readings and LED descriptions are preserved as recorded by Levi.

| Duty | Expected CCR3 | Nominal example, V | Expected from measured endpoints, V | Actual PB10 DC average, V | Signed difference, V | Actual external LED behavior | Result / interpretation |
|---:|---:|---:|---|---|---|---|---|
| 0% | 0 | 0.000 | 0.00 (LOW reference) | 0 | 0.00 (reference) | Completely OFF — reported by Levi September 22 | PASS — functional OFF/LOW check; defines measured LOW endpoint |
| 25% | 250 | 0.825 | 0.82 | 0.82 | 0.00 | dimly lit | Consistent — DC average matches one quarter of measured HIGH; LED is dimly lit |
| 50% | 500 | 1.650 | 1.64 | 1.64 | 0.00 | Lit — reported by Levi September 22 | PASS — functional intermediate output; DC average matches half of measured HIGH |
| 75% | 750 | 2.475 | 2.46 | 2.46 | 0.00 | Lit | Consistent — DC average matches three quarters of measured HIGH and exceeds the 50% reading; LED is lit |
| 100% | 1,000 | 3.300 | 3.28 (HIGH reference) | 3.28 | 0.00 (reference) | Brightly lit | Consistent — highest recorded DC level and brightly lit LED; defines measured HIGH endpoint |

- Measurement source: Levi reported the 0% and 50% results in the September 22 conversation and entered the 25%, 75%, and 100% readings and LED observations directly in this worksheet before requesting the calculations. Values are retained at the precision reported; no extra measurement precision is inferred.
- Calculation: expected average = 0 + (duty / 100) × (3.28 − 0). This gives 0.00, 0.82, 1.64, 2.46, and 3.28 V. Signed difference = recorded DC average − endpoint-based expected average; each difference is 0.00 V at the recorded precision.
- Interpretation: the intermediate DC readings scale linearly with commanded duty relative to the measured endpoints. Together with the reported LED observations, they support the functional five-point output check. The 50% reading is 0.01 V below the nominal 1.65 V example but matches the 1.64 V expectation calculated from the actual 3.28 V HIGH level.
- Limits: the endpoint differences are zero by construction because those measurements define the reference line. Zero intermediate differences at the recorded precision do not establish perfect accuracy, meter uncertainty, pulse frequency, or waveform quality. No numerical accuracy tolerance was declared, so these comparisons are descriptive functional results rather than accuracy certification.
- Initial 0% test: Levi reported the external LED completely OFF and subsequently confirmed a 0 V reading on September 22. External supply-on condition for that earlier OFF test has not been separately confirmed.
- Return to OFF: `Load_Off()`, negative-command limiting, and toggling from 50% were recorded as consistent with OFF in the wrapper table. A separate numerical post-sweep 0% remeasurement was not recorded.
- Visible behavior through reset with all-zero test firmware: no separate result recorded.
- Brief startup glitches measured: not performed.

## Command-state observations

Read these variables at the guide's CHECK breakpoint. Resume before electrical measurements. These getters describe commands, not sensed current or brightness.

September 22: Levi confirmed that `observed_pwm` was `0` and `observed_enabled` was `false` at CHECK for the `Load_SetPWM(0)` test. This records the user-reported debugger observation only; it does not establish the LED state or PB10 voltage.

September 22: [Levi's 50% CHECK screenshot](../images_videos/2026-09-22_pwm_50_command.png) shows `Load_SetPWM(50)`, execution paused at main.c:102 after the observation assignments, `observed_pwm = 50`, and `observed_enabled = true`. The debugger also displays `'2'` beside 50 because the byte value 50 corresponds to the ASCII character '2'; the numeric command remains 50. This screenshot validates command state only; Levi's subsequent LED observation and voltage measurement are recorded separately in the five-point table above.

| Command | Expected observed_pwm | Expected observed_enabled | Actual observed_pwm | Actual observed_enabled | Result |
|---|---:|---|---|---|---|
| 0% | 0 | false | 0 | false | PASS — command-state check, confirmed by Levi September 22 |
| 25% | 25 | true | 25 | true | PASS — recorded by Levi and confirmed consistent September 22 |
| 50% | 50 | true | 50 | true | PASS — September 22 CHECK screenshot |
| 75% | 75 | true | 75 | true | PASS — recorded by Levi and confirmed consistent September 22 |
| 100% | 100 | true | 100 | true | PASS — Levi's updated record reports 100 / true |

An earlier version of the 100% row contained 75. It was flagged for confirmation; Levi's completed record now contains 100 / true and PASS. The final record resolves the discrepancy. No firmware defect or specific cause of the earlier entry was established.

## Wrapper and clamp checks

| Startup command(s) | Expected percentage | Expected enabled | Expected settled LED | Actual command / LED / result |
|---|---:|---|---|---|
| `Load_SetPWM(-10)` | 0 | false | OFF | PASS — Levi recorded consistency with the expected command and LED state |
| `Load_SetPWM(110)` | 100 | true | ON | PASS — Levi recorded consistency with the expected command and LED state |
| `Load_On()` | 100 | true | ON | PASS — Levi recorded consistency with the expected command and LED state |
| `Load_Off()` | 0 | false | OFF | PASS — Levi recorded consistency with the expected command and LED state |
| `Load_SetPWM(50); Load_Toggle()` | 0 | false | OFF | PASS — Levi recorded consistency with the expected command and LED state |
| `Load_Off(); Load_Toggle()` | 100 | true | ON | PASS — Levi recorded consistency with the expected command and LED state |

These results preserve the user's recorded “consistent” observations; separate screenshots or voltage readings were not supplied for each wrapper test. The limits constrain requests to 0–100%. Toggle sends any nonzero setting to OFF and sends OFF to full ON; it does not restore the previous dim setting.

## Optional scope check at 50%

Not performed: an oscilloscope was not available. DC averages support the functional result but cannot establish pulse frequency, HIGH time, edge quality, or exact duty cycle.

| Quantity | Configured expectation | Actual | Instrument / capture |
|---|---:|---|---|
| Frequency | about 1 kHz | not performed | No oscilloscope capture |
| Period | about 1 ms | not performed | No oscilloscope capture |
| HIGH time | about 500 µs | not performed | No oscilloscope capture |
| Duty | about 50% | not performed | No oscilloscope capture |

## Problems and fixes

### Finding CubeMX settings and understanding regeneration

- Difficulty: the instructions used “output polarity” and “initial pulse,” while the UI uses **CH Polarity** and **Pulse (32 bits value)**. It was unclear whether configuration checks required adding code.
- Check: inspect generated TIM2 setup and the PB10 alternate-function configuration.
- Resolution: locate the fields under **TIM2 → Parameter Settings → PWM Generation Channel 3** and make the guide's first step explicitly read-only. The saved configuration already had PSC 83, ARR 999, PWM1, pulse 0, active-HIGH polarity, and PB10 routed to TIM2 channel 3.
- Evidence: source inspection confirmed C0. No additional generated-code edits were required to satisfy that check.

### Connecting the existing load interface to PWM

- Finding: after regeneration, the old module still used GPIO writes and LOAD_CTRL pin labels that were no longer generated. Configuring the timer did not by itself start PWM output.
- Resolution: replace the GPIO implementation with TIM2 compare writes; initialize at zero duty, start channel 3, and preserve the existing ON/OFF/toggle interface. Remove the old blink loop during fixed-duty testing so it cannot overwrite the selected command.
- Evidence: the updated source builds, the debugger reports the selected percentages, and the five DC readings follow the requested duty progression. This was an implementation transition identified during review, not a documented recurring hardware failure.

### Navigating the debugger and distinguishing paused checks from measurements

- Difficulty: locating the macOS Window menu, Breakpoints view, saved debug configuration, and Debug perspective; understanding when to Resume.
- Resolution: open **Window → Show View → Other… → Debug → Breakpoints**, use **stm32_controller Debug**, accept the perspective switch, and Resume from main to CHECK. Read the observation variables at CHECK, then disable the breakpoint and Resume before electrical measurements.
- Evidence: the saved debug-launch and 50% screenshots show a working debug session; the command-state table records expected results. Disabling a breakpoint alone does not resume a paused program.

### Interpreting byte display and the 100% record

- Difficulty: CubeIDE displayed `50 '2'`, raising uncertainty about the percentage. Separately, the 100% worksheet row initially contained 75.
- Resolution: explain that the debugger shows both numeric byte value 50 and its character representation '2'. Flag the 100% mismatch instead of silently changing the data; the user's completed entry now reports 100 / true.
- Evidence: the 50% screenshot and final command-state table. No timer fault was established from either display/recording issue.

### Comparing the voltage with the correct reference

- Question: the measured 50% average was 1.64 V, while the nominal example was 1.65 V; the endpoint-based calculation was initially pending.
- Resolution: use the actual measured HIGH of 3.28 V and LOW of 0 V. Half of that span is 1.64 V. The other intermediate expectations are 0.82 V and 2.46 V.
- Evidence: all three intermediate readings match the endpoint-based expectations at the recorded precision. This is a functional comparison, not a zero-error accuracy specification.

The earlier USB-hub workaround and resistor-probe contact issue are documented in the [bring-up lab](2026-09-04_bringup.md). They were not reported as new failures during this PWM session.

## Final firmware and evidence

- Saved firmware: startup initializes OFF, waits two seconds, records the temporary observation variables, and waits another second. The loop then repeats 0%, 25%, 50%, 75%, and 100%, holding each setting for one second. The former 110% startup command is commented out. The observation variables are startup snapshots (0 / false), not live readouts of the repeating sequence.
- Repeating sequence: installed in the saved source and build-checked. Levi supplied the PWM LED intensity-switching video retained in the private development archive as demonstration evidence. The video is archived and linked. Levi confirmed that the current saved firmware was flashed and that the recording contains two complete cycles; this hardware confirmation is user-reported. It does not establish electrical frequency or duty accuracy.
- Closeout build: current source compiled and linked successfully with 0 errors / 0 warnings; no closeout flash was performed by Codex.
- PWM evidence: [debug launch](../images_videos/2026-09-22_pwm_debug_launch.png), [50% command-state screenshot](../images_videos/2026-09-22_pwm_50_command.png), and the user-recorded tables above.
- Earlier circuit evidence: [breadboard photo](../images_videos/mosfet_bringup.jpg) and bring-up video retained in the private development archive. These are earlier ON/OFF evidence, not a recording of the PWM sweep.
- README updated to describe the measured PWM results, limitations, and saved test configuration.
- Commit scope: TIM2 configuration and generated driver additions, control module, repeating demonstration program, guide, this report, README, two PWM screenshots, and the user-supplied PWM demo video. Local IDE view preferences and the unrelated September 8 note edit are excluded from this change set.
- Suggested commit title: `feat: validate STM32 PWM load control`.
- Commit has not been created as part of this report closeout.

## What I can now explain

- **I changed:** the existing load-control module from software GPIO ON/OFF writes to a hardware timer output. TIM2 channel 3 drives the same PB10/MOSFET/LED circuit, and the application requests a percentage through `Load_SetPWM`.
- **I can explain the timing:** the configured 84 MHz timer clock divided by `(83 + 1)` gives a 1 MHz counter tick. Counting 0 through 999 gives 1,000 ticks per cycle, so the configured frequency is nominally 1 kHz. I did not measure that frequency with a scope.
- **I can explain percentage conversion:** compare = `(ARR + 1) × percent / 100`. At 50%, CCR3 is 500; at 100%, it is 1,000, greater than the maximum counter value of 999. `U` marks an unsigned integer constant; the casts convert the already-clamped percentage to the required unsigned type.
- **I can explain the software state:** the private percentage variable remembers the command. It is not a light or current sensor. `Load_GetPWM` and `Load_IsEnabled` report that record; the multimeter and LED observation provide separate physical evidence.
- **I verified:** five DC voltage/LED test points, all five command-state values, negative and above-100% request limiting, ON/OFF operations, and both toggle directions. The three intermediate voltages match the measured-endpoint calculation at the recorded precision.
- **I can explain the measurement limit:** a 1.64 V DC reading at 50% is an average of a switching signal, not evidence that PB10 produces a constant 1.64 V. The DMM results do not establish exact duty, frequency, switching edges, startup glitches, current-versus-duty behavior, or instrument uncertainty.
- **Resume point:** C0–C8 are complete based on the source checks and recorded/user-confirmed evidence. The PWM demo video is archived, and Levi confirmed the current firmware flash and two complete cycles. Optional oscilloscope measurements remain unperformed; they are separate from the visual demo and do not block this functional milestone.
