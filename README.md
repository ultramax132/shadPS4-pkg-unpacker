<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# PS4 PKG Unpacker

A standalone console program for unpacking PS4 .pkg game files for use with the shadPS4 emulator.

## Features

- Completely unpacks PS4 main .pkg and patch files
- Extracts all game files including `sce_sys` metadata and `eboot.bin`
- Creates output directory structure ready for shadPS4 emulator

## Important

- Please, don't build emulator from this project, use only a prebuilt pkg_unpacker from releases or build it yourself following a instruction specially for your system
- If you want a PS4 emulator use original shadPS4, shadPS4Plus or other forks from community
- Forked from shadPS4Plus
- Created using by AI

## Building

### Prerequisites

- CMake 3.24 or higher
- C++23 compatible compiler
- ZLIB development libraries
- Crypto++ library (or it will be built from externals)

### Build Instructions

```bash
cd pkg_unpacker
mkdir build
cd build
cmake ..
make -j$(nproc)
```

The executable will be created at `build/bin/pkg_unpacker`.

## Usage

```bash
./pkg_unpacker <pkg_file>
```

### Arguments

- `pkg_file`: Path to the .pkg file to unpack to `CUSAxxxxx`

### Examples

```bash
# Unpack to default location (CUSAxxxxx)
./pkg_unpacker game.pkg
```

## Output

The unpacker will create:
- `sce_sys/` directory with game metadata
- All game files including `eboot.bin`
- Complete directory structure ready for shadPS4 emulator

The program will verify that both `sce_sys` and `eboot.bin` are present after extraction.

## Requirements

The unpacked game must contain:
- `sce_sys/` directory
- `eboot.bin` file

These are required for the shadPS4 emulator to run the game.

