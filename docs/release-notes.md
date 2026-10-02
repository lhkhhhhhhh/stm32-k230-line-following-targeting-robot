# GitHub release notes

This repository retains the original school-competition code and result log. The GitHub preparation added the following post-competition cleanup:

- Made the STM32 toolchain prefix and STM32Cube FW path configurable in the Makefile.
- Linked the STM32Cube F1 system template and added the missing SysTick handler required by `HAL_GetTick()` and `HAL_Delay()`.
- Removed a linker-script discard rule that prevented standard runtime and software floating-point routines from linking.
- Clamped steering commands before conversion to `int16_t`, avoiding out-of-range conversion on large derivative spikes.
- Corrected stale encoder pin definitions and line-sensor polarity documentation.
- Kept the laser pin low when the K230 Python main loop exits with an exception, and renamed the earlier 200 ms script to identify it as historical.
- Added build, calibration, status, and safety documentation.

The STM32 firmware now compiles and links with STM32Cube FW F1 v1.8.7 and the GCC 14.3.1 toolchain bundled with STM32CubeIDE 2.1.1. Python syntax was checked for the K230 script. **No post-cleanup hardware test or new competition score is claimed.**

The following competition-era limits remain: the STM32-to-K230 arrival link is not implemented end to end, there is no per-station shot lock, the STM32 uses fixed encoder stop thresholds without wheel-speed feedback, and line-loss recovery has no timeout.
