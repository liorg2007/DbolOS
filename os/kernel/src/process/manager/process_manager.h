#pragma once

#include "memory/paging/paging.h"
#include "filesystem/fat/fat.h"
#include "cpu/idt/isr.h"

#define MAX_GLOB_FD 256
#define MAX_LOCAL_FD 128
#define PROC_KERNEL_STACK_SIZE 2
typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_WAITING,
    PROCESS_TERMINATED
} process_state_t;

typedef struct global_file_descriptor_t {
    int (*_read)(void *buf, uint32_t count, uint32_t off, struct global_file_descriptor_t* glob_fd); // the read function of the fd
    FileData file;
    int ref_count;
    char path[256];
    bool is_dir;
    bool is_used;
    bool is_device;
} global_file_descriptor;

typedef struct file_descriptor_t {
    global_file_descriptor* global_fd;
    uint32_t flags;
    uint32_t offset;
    bool is_used;
} file_descriptor;

typedef struct {
   uint32_t edi, esi, ebp, unused, ebx, edx, ecx, eax;
   uint32_t eip, cs, eflags, esp, ss;
} process_registers_t;

typedef struct {
    uint32_t pid;
    uint32_t terminal_id;
    uint32_t waiting_for;
    bool is_kernel_mode;
    char cwd[256];
    struct page_directory_entry* page_directory;
    void* kernel_stack;
    uintptr_t process_break; // end of process memory, grows with sbrk()
    process_state_t state;
    file_descriptor fd_table[MAX_LOCAL_FD];
    process_registers_t regs;
} process_t;

typedef struct process_node_t {
    process_t proc;
    struct process_node_t* next;
    struct process_node_t* prev;
} process_node_t;

void proc_manager_init();
void init_proc_fd(file_descriptor *fd_table, size_t size);

int create_kernelmode_process(const char *path, int flags);
int create_usermode_process(const char *path, int flags);

int exit_proc(process_node_t* exiting_proc);
void exit_current_process();
process_t* get_current_process();

void force_switch_process();
void wake_up_terminal_processes(uint32_t terminal_id);
void wake_up_waiting_processes(uint32_t wait_for_pid);

void switch_process(struct int_registers* regs);
void copy_registers(const struct int_registers *src, process_registers_t *dst);
void enable_processes();
bool is_schduling();