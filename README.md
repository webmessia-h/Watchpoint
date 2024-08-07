# Watchpoint Kernel Module

<a href="#"><img alt="C" src = "https://img.shields.io/badge/C-black.svg?style=for-the-badge&logo=c&logoColor=white"></a>
<a href="#"><img alt="CMake" src="https://img.shields.io/badge/Make-black?style=for-the-badge&logo=gnu&logoColor=white"></a>

## Description

This Linux kernel module sets a watchpoint on a specified memory address.
When the memory address is accessed (either read or write), the module's callbacks are called, and a backtrace is printed.

## Features

- Monitor a memory address set via module parameter or sysfs entry.
- Set a hardware watchpoint (breakpoint) on the specified memory address.
- Invoke callbacks and print a backtrace when the memory address is accessed.

## Requirements

- Linux kernel version 4.x or higher.
- Yocto build environment for qemux86 or **_native_** x86_64 Linux.

## Installation

> [!CAUTION]
>
> THERE IS ONE VERY IMPORTANT OPTION IN `src/watchpoint.c` FILE:
> IF YOU ARE USING x86 MACHINE, YOU MUST NOT `#DEFINE BREAKPOINT_LENGTH` TO MORE THAN 8 BYTES

### Building with Yocto

1. Ensure you have the following files in your Yocto project:

- `watchpoint.c`: The kernel module source code.
- `COPYING`: The license file.
- `watchpoint.bb`: The Yocto recipe.

> [!IMPORTANT]
>
> Ensure the checksums in LIC_FILES_CHKSUM match those of your COPYING file.

**Build the kernel module using bitbake:**

```sh
bitbake watchpoint
```

### Loading the Module

**Copy the built module to the target device:**

```sh
scp watchpoint.ko root@target_device:/lib/modules/$(uname -r)/extra/
```

**Load the module:**

```sh
insmod /lib/modules/$(uname -r)/extra/watchpoint.ko watch_address=0xYourMemoryAddress
```

**Verify the module is loaded:**

```sh
lsmod | grep watchpoint
```

> [!TIP]
> To unload the module, use `rmmod watchpoint`.

### Installing **NATIVE**

1. Move to the root of cloned git repo and build:

```sh
cd watchpoint-ko/
# I'd recommend using `bear` utility if you want to rewrite/add/delete some code.
# It will create compiler-commands.json, so your IDE won't be all red-underlined.
make # bear -- make
```

2. Insert module:

```sh
sudo insmod watchpoint.ko watch_address="0xYOUR_ADDRESS"
```

**To see logs, use: `sudo dmesg`, I prefer to first run it with `-c` flag to clear the logs,
and then with `-wH` flag so it dynamically prints everything to terminal.**

![LOGS](assets/benchmark/test.png)

## Usage

**Setting the Watchpoint Address**
You can set the watchpoint address through the **sysfs** interface:

```sh
echo 0xYourMemoryAddress > /sys/kernel/watchpoint/watch_address
```

> [!IMPORTANT] > **However there is check mechanism, you better ensure the address is correctly aligned and within a valid range.**

## Example

To set a watchpoint on address **0xffff88013c36a000**, use:

```sh
insmod watchpoint.ko watch_address=0xffff88013c36a000
```

**Or dynamically change the address:**

```sh
echo 0xffff88013c36a000 > /sys/kernel/watchpoint/watch_address
```

## License

## This project is licensed under the GPLv3 License. See the COPYING or LICENSE file for details.
