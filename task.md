# Task #2: Linux Kernel Module

## Description

Write a Linux kernel module that sets a watchpoint to a given memory address. If this memory address is accessed, the module's callbacks are called.

## Functional Requirements

1. **Memory Address Monitoring**:
   - Memory address to monitor is set by a module parameter and can be changed through the sysfs module's entry.
2. **Watchpoint Setting**:

   - The module sets a watchpoint (hardware breakpoint) to a given memory address.

3. **Callback Invocation**:
   - When the memory address is accessed (either read or write), the module's callbacks are called (separate for read and write) and a backtrace is printed.

## Non-Functional Requirements

1. **Build Environment**:
   - The Linux kernel module should be built using Yocto for qemux86.
2. **Yocto Recipe**:

   - The Linux kernel module should have its own Yocto recipe.

3. **Discretionary Requirements**:
   - All other requirements and restrictions are up to your discretion.
