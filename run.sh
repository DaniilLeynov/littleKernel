#!/bin/bash

set -xue

QEMU="qemu-system-riscv32"
CC="clang"
OBJCOPY="llvm-objcopy"  # Или "objcopy", если используете GNU
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding -nostdlib"

# Собираем приложение
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf shell.c user.c common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin
$OBJCOPY -Ibinary -Oelf32-littleriscv --binary-architecture=riscv32 shell.bin shell.bin.o

# Собираем ядро
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf kernel.c common.c shell.bin.o

# Запускаем QEMU
$QEMU -machine virt -m 256M -bios default -nographic -serial mon:stdio --no-reboot -kernel kernel.elf