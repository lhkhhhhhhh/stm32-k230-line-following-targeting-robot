# Line-Following Robot with Vision-Guided Targeting

[简体中文](README.zh-CN.md)

An STM32F103C8T6 sensor-guided line-following robot paired with a CanMV K230 vision module and a two-axis laser-targeting gimbal. This repository documents a 2026 Shanghai Dianji University school competition prototype and the source used to develop it.

> **Project status:** The competition log records **85/100 points and fourth place**. The STM32-to-K230 arrival link did not work during the event. The K230 program therefore fires when its visual alignment condition is met; it has no station confirmation and can re-arm after losing the target. This is a historical prototype, not a verified implementation of every competition rule.

## What is implemented

| Module | Implementation | Current limit |
| --- | --- | --- |
| Line following | Eight digital sensors, weighted line position, steering PID | No measured line-deviation data is included |
| Drive and stopping | TB6612, 20 kHz PWM, TIM2/TIM4 encoder counting, fixed stop ticks | Encoders are not used for wheel-speed feedback |
| Vision and gimbal | K230 rectangle detection with Hough-circle fallback, two UART-controlled servos | Target selection and physical laser-hit error need calibration |
| Laser sequence | Nominal three 500 ms pulses after 15 aligned frames | Loss of alignment can re-arm the sequence |
| Inter-board coordination | STM32 drives PB15; K230 configures a UART RX pin | No working arrival/complete handshake |

The post-competition source cleanup is listed in [release notes](docs/release-notes.md). It has been **compiled, not re-tested on the robot**.

## Repository layout

```text
stm32/                  STM32F103 firmware and GNU Make build
k230/k230_fixed.py      Final competition-era CanMV script (500 ms pulses)
k230/legacy_200ms.py    Earlier historical script; not the final version
docs/比赛总结.txt         Original competition retrospective (Chinese)
docs/release-notes.md   What changed for this GitHub release
```

## Hardware and signal flow

```text
8-channel line sensor ──> STM32F103C8T6 ──> TB6612 ──> left/right motors
wheel encoders       ──> TIM4 / TIM2
                              │ PB15 arrival output (not integrated)
camera ──> CanMV K230 ──> UART1 / UART2 ──> X/Y gimbal servos
                         └── GPIO24 ──> laser control
```

The line sensor's X1–X8 outputs connect to PA7–PA0 respectively. The left encoder uses PB6/PB7 (TIM4), and the right encoder uses PA15/PB3 (TIM2 partial remap). Motor PWM is on PA8 and PA11; direction pins are PA9/PA10 and PB12/PB13; TB6612 standby is PC13. See the source for initialization details.

The STM32 raises PB15 at each encoder stop threshold. In the published K230 script, GPIO41 is configured as UART1 RX, but the main loop never reads it. The boards must be wired and programmed with **one matching protocol** before station-dependent firing can work. K230 pin numbers and FPIOA assignments depend on the actual development board.

## Build the STM32 firmware

Requirements:

- An ARM GNU toolchain providing `arm-none-eabi-gcc`, `objcopy`, and `size`
- GNU Make
- STM32Cube FW F1 **v1.8.7** (HAL, CMSIS, system template, and startup assembly)
- STM32CubeProgrammer for flashing

The build was verified on Windows with the GCC 14.3.1 toolchain bundled with STM32CubeIDE 2.1.1 and STM32Cube FW F1 v1.8.7. The vendor firmware and toolchain are external dependencies and are not copied into this repository.

```sh
make -C stm32 \
  FW_REPO=/path/to/STM32Cube_FW_F1_V1.8.7 \
  TOOLCHAIN_PREFIX=arm-none-eabi- \
  all
```

If the toolchain is not on `PATH`, set `TOOLCHAIN_PREFIX` to the full path ending in `arm-none-eabi-`. `FW_REPO` defaults to `$HOME/STM32Cube/Repository/STM32Cube_FW_F1_V1.8.7`, and can be overridden as above. The outputs are `stm32/build/line_follower.elf`, `.hex`, and `.bin`.

```sh
STM32_Programmer_CLI -c port=SWD -w stm32/build/line_follower.hex -v -rst
```

This build check does not establish that the new firmware has been flashed or validated on the physical robot.

## Run the K230 script

Use a CanMV K230 firmware/IDE installation that provides `media.sensor`, `media.display`, `media.media`, and `cv_lite`. Load `k230/k230_fixed.py` only after checking the actual board's FPIOA mapping, servo IDs, power wiring, camera view, and laser output. `k230/legacy_200ms.py` is retained only to document the earlier iteration.

The competition calibration values are in the source: four encoder thresholds, steering gains, pixel reference `(120, 150)`, a ±25-pixel dead zone, and 20-unit servo steps. They are specific to the original robot and field. Verify the **actual laser spot** at the intended target distance before using those values on another build.

## Safety and known limitations

- **Bench-test with the laser disconnected or replaced by an LED.** The STM32 firmware starts driving after initialization, and the K230 script can trigger laser output from visual alignment alone.
- The K230 script now sets the laser pin low when the Python main loop exits through an exception. It has no independent hardware pulse limiter; a device hang or power fault still needs a physical safety design.
- No station ID, acknowledgement, completion response, or per-station shot counter is implemented. A fixed eight-second stop on STM32 does not guarantee that K230 has finished aiming.
- The line-loss recovery has no timeout. The robot must be physically secured during initial tests.
- A clean firmware build is verified; line accuracy, stopping error, target hit rate, startup behavior, and power compliance have **not** been re-measured after this cleanup.

The [competition retrospective](docs/比赛总结.txt) records the field result and the original integration problem. It does not contain a per-item score sheet, so the reason for each lost point is unknown.

## License

Project-authored source and documentation are available under the [MIT License](LICENSE). STM32Cube FW F1, CanMV firmware, and `cv_lite` remain external dependencies under their respective licenses.
