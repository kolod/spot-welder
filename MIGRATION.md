# PlatformIO to Meson Migration Guide

## Overview
This project has been converted from PlatformIO to Meson build system for the STC8H1K08 microcontroller.

## Key Changes

### Old Configuration (platformio.ini)
- Platform: intel_mcs51 @ 2.2.0
- Toolchain: sdcc @ 1.40100.12072
- Upload tool: stcgal @ 1.110.0
- Board: STC8H1K08

### New Configuration (meson.build)
Meson provides a more portable and flexible build system. Configuration is split between:
- **meson.build** - Build definitions and project structure
- **meson_options.txt** - Configurable options (board, memory sizes)

## Prerequisites

Install required tools:

```bash
# Install Meson build system
pip install meson ninja

# Install SDCC toolchain (for 8051 compilation)
# On Windows: Download from https://sourceforge.net/projects/sdcc/files/
# Or use package manager:
# - Ubuntu/Debian: sudo apt-get install sdcc
# - macOS: brew install sdcc
# - Windows: choco install sdcc (via Chocolatey)

# Install stcgal for flashing
pip install stcgal

# Install rich for formatted memory usage output
pip install rich
```

## Configure and Build

Ongoing configure/build instructions are maintained in `README.md` under the **Building and Flashing** section.

This migration guide focuses on what changed during the move from PlatformIO to Meson.

## Flashing the Device

Using stcgal (requires libusb):

```bash
# Flash the device
stcgal -p /dev/ttyUSB0 -P stc8g build/spot-welder.ihx

# On Windows, replace /dev/ttyUSB0 with COM port (e.g., COM3)
stcgal -p COM3 -P stc8g build/spot-welder.ihx
```

## File Structure

```
spot-welder/
├── meson.build          # Root build configuration
├── meson_options.txt    # Build options
├── src/
│   ├── main.c          # Main source code
│   └── 8h1k08.h        # Device header file
├── include/            # Additional headers
├── lib/                # Libraries
├── docs/               # Documentation
└── build/           # Build output (created by meson setup)
```

## Adding More Source Files

Edit `meson.build` and add files to the `input` list in the `custom_target`:

```meson
input: files(
  'src/main.c',
  'src/other.c',
),
```

## Troubleshooting

### SDCC not found
- Ensure SDCC is installed and in PATH
- Verify with: `sdcc --version`

### Meson still uses GCC
- This project uses a Meson custom target that invokes SDCC directly
- Ensure SDCC is installed and available in PATH
- Reconfigure with: `meson setup build --reconfigure`
- If needed, recreate build dir (see build section)

### stcgal connection issues
- Check USB/serial connection
- Verify correct COM port
- On Linux, may need: `sudo usermod -a -G dialout $USER`

### Build fails with memory errors
- Check flash and RAM sizes in `meson_options.txt`
- Verify with device datasheet (STC8H1K08: 8KB flash, 1KB RAM)

## Further Reference

- [Meson Documentation](https://mesonbuild.com/)
- [SDCC Documentation](http://sdcc.sourceforge.net/)
- [STC8H1K08 Datasheet](http://www.stcaimcu.com/)
