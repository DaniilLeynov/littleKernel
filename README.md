# Simple OS Kernel for RISC-V

A minimalistic operating system kernel for RISC-V (32-bit) architecture, currently under active development. Implements basic process management, memory management, and system call mechanisms.

## Core Components

### Multitasking
- Context switching between processes
- Round-robin scheduler (in development)

### Memory Management
- Paged memory organization (4 KiB pages)
- Process address space isolation
- **Bump allocator** for physical memory management:
  - Linear allocation with fixed offset


### System Calls
- `SYS_PUTCHAR` - character output to terminal
- `SYS_GETCHAR` - character input from terminal
- `SYS_EXIT` - process termination

### Command Shell (Under Development)
- Basic system interaction interface
- Application launching and process management

---

## Requirements

### Development Environment
- LLVM/Clang ≥ 12.0 (with RISC-V 32-bit support)
- llvm-objcopy utility
- QEMU ≥ 6.0 (`qemu-system-riscv32`)

---

## Build and Run

```bash
# Clone repository
git clone https://github.com/DaniilLeynov/littleKernel.git
cd littleKernel/

# Build and start emulation
./run.sh
