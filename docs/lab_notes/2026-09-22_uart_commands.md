# September 22 — UART Command Interface Validation

## Objective

Control the existing NUCLEO-F446RE / TIM2 / PB10 / 2N7000 / external LED load from a computer over the Nucleo ST-Link virtual serial connection, without changing or recompiling the requested PWM setting.

## Hardware and Connection

- Controller: ST NUCLEO-F446RE / STM32F446RET6.
- Load: previously validated PB10/D6 PWM output, 220 ohm gate resistor, 2N7000 low-side switch, externally powered LED and series resistor, and common ground.
- Serial peripheral: USART2 asynchronous mode.
- USART2 pins: PA2/TX and PA3/RX, connected through the Nucleo ST-Link virtual COM port.
- Serial settings: 115200 baud, 8 data bits, no parity, 1 stop bit, and no hardware flow control.
- Computer connection: Nucleo connected to macOS through the working USB hub path.
- Terminal used: macOS `screen` connected to `/dev/cu.usbmodem...` at 115200 baud.

The installed STM32CubeIDE Terminal view offered Local, SSH, and Telnet terminals but did not offer a serial terminal. The [saved menu capture](../images_videos/2026-09-22_cubeide_terminal_choices.png) records that limitation. The external macOS terminal provided the serial connection instead.

## Firmware Changes

- [stm32_controller.ioc](../../firmware/stm32_controller/stm32_controller.ioc) enables USART2 in asynchronous mode while retaining TIM2 channel 3 on PB10.
- Generated initialization configures USART2 for 115200 baud, 8-bit words, one stop bit, no parity, transmit and receive, no hardware flow control, and 16-times oversampling.
- [communications.h](../../firmware/stm32_controller/Core/Inc/communications.h) declares the public initialization and processing functions.
- [communications.c](../../firmware/stm32_controller/Core/Src/communications.c) implements a 32-byte line buffer, one-byte polling reception, command parsing, load-control calls, and bounded text responses.
- [main.c](../../firmware/stm32_controller/Core/Src/main.c) initializes TIM2 and USART2, initializes the load at zero duty, passes `huart2` to the communications module, and repeatedly processes serial input.

The communications code receives one byte at a time with a 10 ms maximum wait. A carriage return or line feed ends the command. The receive buffer reserves one byte for the C-string terminator. Commands longer than the buffer are discarded with `ERR line`.

## Command Protocol

| Command or event | Expected response | Expected load result |
|---|---|---|
| Board reset | `READY` | Commanded PWM begins at 0% |
| `PWM 0` | `OK PWM=0` | LED OFF |
| `PWM 25` | `OK PWM=25` | LED dimmer than full output |
| `PWM 50` | `OK PWM=50` | Intermediate output |
| `PWM 75` | `OK PWM=75` | Higher intermediate output |
| `PWM 100` | `OK PWM=100` | LED at full commanded output |
| `STATUS` after `PWM 25` | `STATUS pwm=25 enabled=1` | No load change |
| `OFF` | `OK PWM=0` | LED OFF |
| `PWM -1` | `ERR range` | Previous setting retained |
| `PWM 101` | `ERR range` | Previous setting retained |
| Malformed or unknown command | `ERR command` | Previous setting retained |

## Recorded Result

Levi reported on September 22 that the UART implementation, connection, and command testing were complete and working. The archived terminal screenshot and physical demonstration video were subsequently inspected and linked below.

- Build: the current source, including the explanatory comments added after the hardware session, was rebuilt from the saved Debug configuration on September 22. Compilation and linking completed with zero errors and zero warnings. A separate build screenshot has not been saved.
- Flash: reported complete by Levi for the tested UART firmware. The later comment-only source edit did not change executable statements, but that edited snapshot has not been reflashed as a separate hardware test.
- Terminal responses: the [terminal capture](../images_videos/2026-09-22_uart_terminal.png) visibly contains `READY`, `STATUS pwm=0 enabled=0`, `OK PWM=25`, `STATUS pwm=25 enabled=1`, `ERR command`, `ERR range`, and `OK PWM=0`.
- Physical behavior: the [13.9-second UART demonstration](../images_videos/2026-09-22_uart_led_control.mp4?raw=1) frames the terminal and external breadboard LED together. The response sequence progresses through 0%, 25%, 100%, and 0%; the LED is initially OFF, becomes visibly illuminated at the nonzero settings, and returns OFF after the final zero response.
- Invalid-command and range-error tests: the screenshot and video visibly include `ERR command` and `ERR range`. Because macOS `screen` did not locally echo typed characters, the captures show firmware responses rather than the corresponding input text. The exact input association is retained from Levi's test report and the implemented parser.
- New electrical measurements: not performed or supplied for this UART session. The earlier [PWM validation](pwm_validation.md) contains the prior PB10 DC-average measurements and must not be presented as new UART-session measurements.

The milestone is **functionally complete with archived terminal and physical-load evidence**. The media demonstrate serial responses and visible load behavior; they do not replace electrical measurements or show the hidden terminal input characters.

## Preserved Evidence

- [UART terminal response capture](../images_videos/2026-09-22_uart_terminal.png): records startup readiness, initial and 25% status, accepted PWM/OFF responses, a malformed-command error, and an out-of-range error.
- [UART terminal and LED demonstration](../images_videos/2026-09-22_uart_led_control.mp4?raw=1): 13.9-second continuous phone video framing both the response terminal and the external LED while the output moves through zero and nonzero PWM settings. The public copy was re-encoded to a 6.6 MB MP4 and stripped of phone location and device metadata.
- [CubeIDE Terminal choices](../images_videos/2026-09-22_cubeide_terminal_choices.png): records why the macOS `screen` workaround was used.
- Saved firmware and configuration: record the USART2 setup, parser, responses, and load-control calls.
- Local rebuild after the explanatory comment edit: zero errors and zero warnings.

The success screenshot does not display entered commands because local echo was disabled. The video mitigates this limitation by showing new firmware response lines and the physical LED changing in the same continuous recording.

## Problems and Fixes

### CubeIDE did not offer a serial-terminal connection

- Symptom: the Terminal connection menu contained Local Terminal, SSH Terminal, and Telnet Terminal, with no Serial Terminal entry.
- Evidence: [CubeIDE Terminal choices](../images_videos/2026-09-22_cubeide_terminal_choices.png).
- Workaround: identify the Nucleo virtual COM device under `/dev/cu.usbmodem...` and open it with macOS `screen` at 115200 baud.
- Result: Levi reported that the connection and UART command session worked through the macOS terminal.

### Separating commands from numbers

- Implementation detail: the parser first matches the four-character prefix `PWM `.
- `command + 4` then points at the first numeric character. For `PWM 25`, it points at `25`.
- `strtol` converts that text to a signed number and reports where parsing stopped, allowing malformed text and out-of-range values to be rejected before changing the load.

## Limitations

- Reception and transmission are blocking, with short timeouts. This is suitable for the current interactive command rate but has not been evaluated for high-rate telemetry.
- The terminal input is not echoed by the firmware, so typed characters may be invisible in terminal programs that do not provide local echo.
- The module records commanded PWM state. It does not sense actual voltage, current, LED brightness, or MOSFET operation.
- No oscilloscope, UART timing, throughput, long-command stress, or power-cycle glitch test was performed for this session.
- No INA219 or other sensor telemetry is implemented yet.

## Conclusion

USART2 now provides a line-based command interface for the validated PWM load stage. The saved firmware supports percentage commands, OFF, commanded-state reporting, range rejection, malformed-command rejection, and a startup READY message. Levi reported that the serial connection and integrated command control worked on the physical system through macOS `screen`.

The terminal screenshot and terminal-plus-LED video are archived and linked. Together with the saved source, successful rebuild, and Levi's observations, they close the UART milestone and make it ready for a focused Git commit. The next development milestone is Python serial logging; sensor data should wait until the logger can reliably capture the existing PWM telemetry.
