#pragma once

typedef unsigned int paddr_t;
typedef unsigned int vaddr_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define PAGE_SIZE 4096
#define va_list  __builtin_va_list
#define va_start __builtin_va_start
#define va_end   __builtin_va_end
#define va_arg   __builtin_va_arg
#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX - 1)
#define SYS_PUTCHAR 1
#define SYS_GETCHAR 2
#define SYS_EXIT    3

void printf(const char *fmt, ...);
void *memset(void *buf, char c, int n);
void *memcpy(void *dest, const void *src, int n);
int strcmp(const char *s1, const char *s2);