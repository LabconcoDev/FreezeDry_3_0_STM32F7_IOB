# Dev Container Setup Guide

## Overview

This dev container provides a complete development environment for **STM32F723 firmware development** using the **Zephyr RTOS** (v4.4.0 LTS). It includes all necessary tools, compilers, and dependencies pre-configured for seamless collaboration.

## Prerequisites

Before opening the project in the dev container, ensure you have:

1. **Docker** — Installed and running
   - [Install Docker Desktop](https://www.docker.com/products/docker-desktop)
   - Verify: `docker --version`

2. **Visual Studio Code** — Latest version
   - [Download VS Code](https://code.visualstudio.com/)

3. **Remote - Containers Extension** — Install in VS Code
   - Open VS Code → Extensions → Search "Remote - Containers" → Install

## Quick Start

### Step 1: Open in Dev Container
```bash
# Clone/open the repository in VS Code
code .

# Press: Ctrl+Shift+P (or Cmd+Shift+P on Mac)
# Type: "Dev Containers: Reopen in Container"
# Press Enter
```

### Step 2: Wait for Initialization
- Container builds automatically (first time: 2-5 minutes)
- The `init-workspace.sh` script runs automatically and:
  - Downloads Zephyr RTOS v4.4.0 source code
  - Sets up West package manager
  - Scaffolds the `my-app` directory with sample firmware
  - Exports Zephyr environment variables

### Step 3: Start Developing
```bash
# Build the sample application
cd workspace/my-app
west build -b stm32f723e_disco

# Flash to device (if connected)
west flash
```

## What's Included

### Dockerfile
- **Base Image**: `ghcr.io/zephyrproject-rtos/zephyr-build:latest`
- **Additional Tools**:
  - USB utilities for device communication
  - Git for version control
  - User namespace support for safe execution

### VS Code Extensions
- **ms-vscode.cpptools** — C/C++ IntelliSense
- **ms-vscode.cpptools-extension-pack** — C/C++ tools bundle
- **twxs.cmake** — CMake syntax highlighting
- **ms-vscode.cmake-tools** — CMake integration
- **trond-snekvik.devicetree** — Device tree file support
- **marus25.cortex-debug** — ARM Cortex debugger

### Initialization Script (`init-workspace.sh`)
Automatically runs on container creation and:
1. **Initializes West workspace** with Zephyr v4.4.0
2. **Pulls all modules** (HALs, crypto libraries, bootloaders)
3. **Exports Zephyr environment** variables
4. **Scaffolds `my-app`** with CMakeLists.txt, prj.conf, and main.c template

## Environment Variables

The following are automatically configured:

```bash
ZEPHYR_BASE=/workspaces/workspace/zephyr
```

## Hardware Setup

### STM32F723E Discovery Board
1. Connect via USB to your host machine
2. Verify detection: `lsusb` (Linux/Mac) or Device Manager (Windows)
3. Flash firmware:
   ```bash
   cd workspace/my-app
   west build -b stm32f723e_disco
   west flash
   ```

### USB Pass-through
The container runs with `--privileged` flag to access USB devices directly. Ensure your board is connected **before** or **after** starting the container.

## File Structure

```
.devcontainer/
├── Dockerfile           # Base image + dependencies
├── devcontainer.json    # VS Code configuration
├── init-workspace.sh    # Automatic workspace setup script
└── README.md           # This file

workspace/
├── zephyr/             # Zephyr RTOS source (auto-downloaded)
├── my-app/             # Your application (auto-scaffolded)
├── bootloader/         # MCUBoot bootloader
├── modules/            # External HALs and libraries
└── tools/              # Development utilities
```

## Common Tasks

### Clean Build
```bash
cd workspace/my-app
west build -c -b stm32f723e_disco
```

### Update Zephyr Modules
```bash
cd workspace
west update
```

### Check Device Connected
```bash
lsusb  # or check Device Manager on Windows
```

### View Build Output
```bash
cat workspace/my-app/build/build.log
```

## Troubleshooting

### Container Won't Start
- Ensure Docker is running: `docker ps`
- Check disk space: `docker system df`
- Clear Docker cache: `docker system prune`

### USB Device Not Found
- Ensure device is connected **before** container starts
- Run container with `--privileged` flag (already configured)
- On Windows, use Docker Desktop's USB settings
- On Mac, ensure Docker Desktop has USB pass-through enabled

### West Command Not Found
- Verify initialization completed: Check terminal output for "Workspace Initialization Complete"
- Manually initialize: 
  ```bash
  cd workspace
  west init -m https://github.com/zephyrproject-rtos/zephyr --mr v4.4.0 .
  west update
  ```

### Permission Denied Errors
- The container runs as non-root user (`user`)
- Built files may need permission reset: `sudo chown -R user:user workspace/`

### Out of Memory
- Reduce parallel build jobs: `west build -j 2`
- Default is 4 parallel jobs

## Sharing & Collaboration

This setup is fully portable. To share with team members:

1. **Commit to repository**:
   ```bash
   git add .devcontainer/
   git commit -m "Add dev container setup"
   git push
   ```

2. **Team members clone and open**:
   ```bash
   git clone <repo>
   code .
   # Ctrl+Shift+P → "Dev Containers: Reopen in Container"
   ```

3. **Environment is identical** across all machines ✓

## References

- [Zephyr Documentation](https://docs.zephyrproject.io/)
- [Zephyr v4.4.0 Release](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.4.0)
- [STM32F723E Discovery Board](https://www.st.com/en/evaluation-tools/32f723ediscovery.html)
- [VS Code Dev Containers](https://code.visualstudio.com/docs/devcontainers/containers)
- [Docker Documentation](https://docs.docker.com/)

## Support

For issues or questions:
1. Check troubleshooting section above
2. Review Zephyr documentation
3. Check VS Code Dev Container logs: View → Output → "Dev Containers"
