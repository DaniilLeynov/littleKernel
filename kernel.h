#pragma once
typedef unsigned int vaddr_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

#define NULL ((void*)0)
#define PROCESS_MAX 8
#define PROCESS_UNUSED 0
#define PROCESS_USED 1
#define SATP_SV32 (1u << 31)
#define PAGE_V    (1 << 0)   
#define PAGE_R    (1 << 1)   
#define PAGE_W    (1 << 2)   
#define PAGE_X    (1 << 3)   
#define PAGE_U    (1 << 4)   
#define USER_BASE 0x1000000
#define SSTATUS_SPIE (1 << 5)
#define SCAUSE_ECALL 8
#define PROC_EXITED   2

struct trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t tp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t sp;
} __attribute__((packed));


struct PCB{
    int pid;
    int state;
    vaddr_t sp;
    uint32_t *page_table;
    uint8_t kernel_stack[8192];
};

struct sbiret{
    long error;
    long value;
};

void user_entry(void);
long getchar(void);
struct PCB *create_process(const void *image, int image_size);
void switch_context(uint32_t *prev_sp, uint32_t *next_sp);
void yield(void);
void map_page(uint32_t *table1, uint32_t vaddr, vaddr_t paddr, uint32_t flags);
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, 
                      long arg4, long arg5, long fid, long eid);
void putchar(char ch);
vaddr_t palloc(uint32_t n);
void proc_a_entry(void);
void proc_b_entry(void);
void handle_syscall(struct trap_frame *f);
void kernel_main(void);
void boot(void);