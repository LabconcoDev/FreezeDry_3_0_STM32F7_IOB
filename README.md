# my-app Firmware Starter (Freeze Dry 3.0 Baseline)

This folder is the starting point for Freeze Dry 3.0 firmware implementation on Zephyr RTOS.

It is designed to be a reusable base that includes:
- Zephyr app build structure
- Board-targeted build scripts
- Task-based application skeleton
- Platform abstraction layer for portability

## Scope and Intent

Use this project as the baseline firmware foundation, then add product features on top.

Current app identity:
- Project: `stm32f723_app`
- Default board: `lcc_rpi_carrier_stm32f723`
- Path: `my-app`

## Repository Structure (Relevant Parts)

- `.devcontainer/`: Dev Container definition and bootstrap scripts
- `.vscode/`: Team build tasks, debug launch config
- `my-app/`: Zephyr application source and config

## .devcontainer Files

### `.devcontainer/devcontainer.json`
Defines the container runtime and VS Code integration.

Highlights:
- Uses `.devcontainer/Dockerfile`
- Installs recommended extensions:
  - C/C++ tools
  - CMake tools
  - DeviceTree support
  - Cortex Debug
- Runs container as non-root user `user`
- Enables USB access with:
  - `--privileged`
  - mount `/dev/bus/usb`
- Runs `postCreateCommand` to execute `.devcontainer/init-workspace.sh`

### `.devcontainer/Dockerfile`
Container image build instructions.

Highlights:
- Base image: `ghcr.io/zephyrproject-rtos/zephyr-build:latest`
- Adds packages: `usbutils`, `git`
- Keeps normal development under user `user`

### `.devcontainer/init-workspace.sh`
Executed automatically after container creation.

What it does:
- Initializes `west` for Zephyr v4.4.0 when needed
- Runs `west update` and `west zephyr-export`
- Creates `my-app` scaffold only if missing

Note:
- Existing `my-app` is not overwritten by the scaffold section.

## .vscode Files

### `.vscode/tasks.json`
Defines team build tasks in VS Code:
- `Build (Incremental)` (default build task)
- `Build (Clean)`

### `.vscode/launch.json`
Cortex-Debug launch configuration.

Highlights:
- Debug type: `cortex-debug`
- External GDB server target: `host.docker.internal:2331`
- ELF target: `my-app/build/zephyr/zephyr.elf`
- RTOS awareness: `Zephyr`

### `host-tools/start-jlink-gdbserver.ps1` and `.cmd`
Host-side helper to start SEGGER J-Link GDB Server.

Default server ports:
- GDB: `2331`
- SWO: `2332`
- Telnet: `2333`

## Build Instructions

## 1) Build from VS Code Task (recommended)

In VS Code:
- Run task `Build (Incremental)` for normal development
- Run task `Build (Clean)` when changing toolchain/config significantly

## 2) Direct west commands

```bash
cd zephyr_workspace
west build -b lcc_rpi_carrier_stm32f723 -d ../my-app/build ../my-app
west build -p always -b lcc_rpi_carrier_stm32f723 -d ../my-app/build ../my-app
```

## Build Outputs

After successful build:
- `my-app/build/zephyr/zephyr.elf`
- `my-app/build/zephyr/zephyr.bin`
- `my-app/build/zephyr/zephyr.hex`

## Debug Instructions (J-Link + VS Code)

1. On host machine, start J-Link GDB server using:
   - `host-tools/start-jlink-gdbserver.cmd` or
   - `host-tools/start-jlink-gdbserver.ps1`
2. Ensure it listens on port `2331`.
3. In VS Code, run debug profile:
   - `Zephyr STM32F723 (J-Link external on host)`

## Source Layout (my-app)

- `CMakeLists.txt`: app build source list and include directories
- `prj.conf`: Zephyr Kconfig options
- `boards/`: board-specific definitions
- `src/main.c`: app entry point
- `src/task_manager.*`: task orchestration
- `src/task1_handler.c`, `src/task2_handler.c`: functional task handlers
- `src/platform/`: platform abstraction (`platform_os`, `platform_gpio`)

## How to Expand Toward Freeze Dry 3.0

Recommended next layers:
- Add platform modules: ADC, I2C, SPI, UART, timers, watchdog, NVM
- Add services: diagnostics, data logging, settings storage, comms stack
- Add application modules: state machine, control loops, fault manager

Suggested folder growth under `src/`:
- `src/services/`
- `src/drivers/`
- `src/app/`
- `src/common/`

## Team Usage Notes

- Treat this folder as a baseline template for feature branches.
- Keep hardware-specific code inside `src/platform/` and `boards/`.
- Keep product logic independent from low-level hardware details.
- Prefer adding new modules instead of expanding single large source files.

## Quick Start Checklist

1. Reopen repository in Dev Container.
2. Confirm `west` setup completed. Confirm west command returns something!
3. Run Terminal->Run Task -> `Build (Clean / Incremental)`.
4. Verify artifact: `my-app/build/zephyr/zephyr.elf`.
5. Start J-Link server on host and begin debug session if needed.
