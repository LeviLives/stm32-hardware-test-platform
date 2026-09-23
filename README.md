# Embedded Hardware Test & Control Platform

An embedded hardware test-and-control project built around the NUCLEO-F446RE. The current prototype demonstrates a MOSFET-switched external LED, modular C load control, hardware-timer PWM checked with a multimeter and ST-Link debugger, and UART commands from a computer. Sensor measurement and Python automation are planned extensions.

## Validated Prototype

The same external LED circuit used for GPIO bring-up now supports percentage-based load commands:

```text
macOS terminal -> ST-Link virtual COM -> USART2 command parser
                                      -> TIM2/PB10 PWM -> 2N7000 -> external LED
```

![STM32 and MOSFET breadboard prototype](docs/images_videos/mosfet_bringup.jpg)

TIM2 channel 3 is configured for nominal **1 kHz** using an 84 MHz timer clock, prescaler 83, and period 999. September 22 measurements at PB10 were:

| Commanded duty | Measured DC average |
|---:|---:|
| 0% | 0 V |
| 25% | 0.82 V |
| 50% | 1.64 V |
| 75% | 2.46 V |
| 100% | 3.28 V |

The LED was OFF at 0% and lit at the nonzero settings. Intermediate averages match the proportions calculated from the measured 0 V / 3.28 V endpoints at the recorded precision. All five command-state checks and six ON/OFF, toggle, and limit checks were reported as consistent with expectations. No oscilloscope measurements were performed: frequency is configured, not measured, and the DC comparison is not an accuracy certification.

See the [PWM validation report](docs/lab_notes/pwm_validation.md) for calculations, observations, debugging, and limitations, or the [step-by-step guide](docs/pwm_walkthrough.md) to reproduce the tests.

**Saved firmware behavior:** this revision initializes the load at 0%, sends `READY` over USART2 at 115200 baud, and waits for line-based commands. `PWM 0..100` selects the duty setting, `OFF` commands 0%, and `STATUS` reports the stored PWM and enabled state. Malformed and out-of-range requests return an error without applying a new setting.

The [UART validation report](docs/lab_notes/2026-09-22_uart_commands.md) records the protocol, implementation, limitations, and results. The [terminal capture](docs/images_videos/2026-09-22_uart_terminal.png) shows startup, state, accepted-command, malformed-command, and range-error responses. A [13.9-second physical demonstration](docs/images_videos/2026-09-22_uart_led_control.MOV) frames the response terminal and external LED together while the output moves through 0%, 25%, 100%, and back to 0%. Typed commands are not visible because the macOS `screen` session did not use local echo.

## Verified Status

- [x] Repository folder structure created
- [x] STM32CubeMX/CubeIDE project generated for `NUCLEO-F446RE`
- [x] Onboard LED2 blink firmware builds and a Debug ELF exists
- [x] ST-Link is detected when the Nucleo is connected through a USB hub
- [x] Flash and visually confirm LED2 blink on the physical board
- [x] Calculate and build the external 2N7000/LED circuit
- [x] Measure GPIO, gate, drain, and LED current
- [x] Verify the gate-pulldown safe state
- [x] Add the breadboard photo and complete the bring-up lab note
- [x] Make the first focused Git commit
- [x] Validate the five modular load-control functions and record the final blink behavior ([September 8 evidence](docs/lab_notes/2026-09-08_load_control.md))
- [x] Implement TIM2 PWM control and validate five duty settings with DC measurements
- [x] Check PWM command-state reporting, ON/OFF, toggle, and request limits
- [x] Flash and record the final repeating PWM demonstration
- [x] Add USART2 commands for PWM, OFF, STATUS, malformed input, and range errors
- [x] Control the physical LED from a computer and archive terminal and video evidence

The MOSFET/LED bring-up has been physically verified and documented with
measured voltage, current, resistor values, pull-down behavior, and media.

## Planned Capabilities

- Python CSV logger and automated PWM sweep
- Periodic UART telemetry for automated logging
- INA219 voltage/current sensing
- Temperature and calculated-power telemetry
- Safe fault shutdown and reset state machine
- Altium schematic, PCB layout, BOM, and manufacturing outputs
- Defined validation plan, measured results, plots, and demo material

## Public Repository Scope

This public snapshot contains the firmware, calculations, lab notes, and sanitized evidence needed to reproduce the documented results. Original phone media with embedded location metadata remains only in the private development repository.
