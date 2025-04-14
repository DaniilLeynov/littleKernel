#include "kernel.h"
#include "common.h"

#define ERR(fmt, ...)                                                        \
    do {                                                                       \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {}                                                           \
    } while (0)


extern char __bss[], __bss_end[], __stack_top[], __kernel_base[], _binary_shell_bin_start[], _binary_shell_bin_size[];
extern char  __free_RAM[], __free_RAM_end[];
struct PCB process_array[PROCESS_MAX];
struct PCB *current_proc;
struct PCB *idle_proc;
struct PCB *proc_a;
struct PCB *proc_b;


__attribute__((naked)) 
void user_entry(void) {
    __asm__ __volatile__(
        "csrw sepc, %[sepc]        \n"
        "csrw sstatus, %[sstatus]  \n"
        "sret                      \n"
        :
        : [sepc] "r" (USER_BASE),
          [sstatus] "r" (SSTATUS_SPIE)
    );
}

long getchar(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    return ret.error;
}

struct PCB *create_process(const void *image, int image_size){
    struct PCB *proc = NULL;
    int count;
    for(count = 0; count < PROCESS_MAX; ++count){
        if(process_array[count].state == PROCESS_UNUSED){
            proc = &process_array[count];
            break;
        }
    }
    if(proc == NULL) ERR("no free process space");

    uint32_t *sp = (uint32_t *) &proc->kernel_stack[sizeof(proc->kernel_stack)];
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = (uint32_t) user_entry;  // ra

    uint32_t *page_table = (uint32_t *) palloc(1);

    for(paddr_t paddr = (paddr_t) __kernel_base;
        paddr < (paddr_t) __free_RAM_end; 
        paddr += PAGE_SIZE){

        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
    }

    for (int off = 0; off < image_size; off += PAGE_SIZE) {
        paddr_t page = palloc(1);

        
        size_t remaining = image_size - off;
        size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

     
        memcpy((void *) page, image + off, copy_size);
        map_page(page_table, USER_BASE + off, page,
                 PAGE_U | PAGE_R | PAGE_W | PAGE_X);
    }
    proc->pid = count + 1;
    proc->state = PROCESS_USED;
    proc->sp = (uint32_t) sp;
    proc->page_table = page_table;
    return proc;
}

__attribute__((naked)) 
void switch_context(uint32_t *prev_sp, uint32_t *next_sp){
     __asm__ __volatile__(
        "addi sp, sp, -13 * 4\n" 
        "sw ra,  0  * 4(sp)\n"   
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"

        // Switch 
        "sw sp, (a0)\n"         
        "lw sp, (a1)\n"         
       
        "lw ra,  0  * 4(sp)\n"  
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"  
        "ret\n"
    );
}

void yield(void){
    struct PCB *next = idle_proc;
    for(int i = 0; i< PROCESS_MAX; ++i){
        struct PCB *proc = &process_array[(current_proc->pid + i) % PROCESS_MAX]; //guarantee that we will not go beyond the stack boundary
        if(proc->state == PROCESS_USED && proc->pid >0){
            next = proc;
            break;
        }
    }

    if(next == current_proc) return;

    // Context switch
    struct PCB *tmp = current_proc;
    current_proc = next;
    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
         :
        
        : [satp] "r" (SATP_SV32 | ((uint32_t) next->page_table / PAGE_SIZE))
    );
    switch_context(&tmp->sp, &next->sp);
}

void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
    if (!is_aligned(vaddr, PAGE_SIZE))
        ERR("unaligned vaddr %x", vaddr);

    if (!is_aligned(paddr, PAGE_SIZE))
        ERR("unaligned paddr %x", paddr);

    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    if ((table1[vpn1] & PAGE_V) == 0) {
        uint32_t pt_paddr = palloc(1);
        table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
    }

    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
    uint32_t *table0 = (uint32_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

struct sbiret sbi_call(long arg0,long arg1,long arg2,long arg3,long arg4,long arg5, long fid,long eid){
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
                        : "=r"(a0), "=r"(a1)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}


paddr_t palloc(uint32_t n){
    static paddr_t next_p = (paddr_t) __free_RAM;
    paddr_t paddr = next_p;
    next_p += n*PAGE_SIZE;
    if (next_p > (paddr_t) __free_RAM_end)
    {
        printf("%d %d", next_p, __free_RAM_end);
        ERR("out of memmory");
    }
    void * tmp = (void*) paddr;
    memset(tmp, 0, n*PAGE_SIZE);
    return paddr;
}


void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        yield();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        yield();
    }
}

void handle_syscall(struct trap_frame *f) {
    switch (f->a3) {
        case SYS_PUTCHAR:
            putchar(f->a0);
            break;
        case SYS_GETCHAR:
            while (1) {
                long ch = getchar();
                if (ch >= 0) {
                    f->a0 = ch;
                    break;
                }

                yield();
            }
            break;
        case SYS_EXIT:
            printf("process %d exited\n", current_proc->pid);
            current_proc->state = PROC_EXITED;
            yield();
            ERR("unreachable");
        default:
            ERR("unexpected syscall a3=%x\n", f->a3);
    }
}

void kernel_main(void){
    memset(__bss, 0, (int)__bss_end - (int) __bss);

    idle_proc = create_process(NULL, 0);
    idle_proc->pid = 0; 
    current_proc = idle_proc;
    create_process(_binary_shell_bin_start, (int) _binary_shell_bin_size);

    

    yield();
    ERR("switched to idle process");
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void){
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "j kernel_main\n"
        :
        :[stack_top] "r" (__stack_top)
    ); 
}
