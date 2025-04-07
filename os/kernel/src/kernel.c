#include "drivers/vga/vga.h"
#include "cpu/gdt/gdt.h"
#include "cpu/idt/idt.h"
#include "cpu/pit/pit.h"
#include "memory/physical/multiboot.h"
#include "memory/physical/physical_memory_manager.h"
#include "memory/paging/paging.h"
#include "memory/heap/heap.h"
#include "drivers/harddisk/ata/ata.h"
#include "filesystem/fat/fat.h"
#include "drivers/keyboard/keyboard.h"
#include "process/manager/process_manager.h"
#include "process/syscalls/syscalls.h"
#include "terminal/terminal_manager.h"
#include "process/syscalls/handlers/time/time.h"

#include <fcntl.h>

extern void jump_usermode(process_registers_t *addr);
void system_startup_animation();

void kernel_physical_end(void);
void kmain(multiboot_info_t *mbi)
{
    asm("cli");
    gdt_init();
    vga_init();

    idt_init();
    vga_putstring("IDT Initialized\n");
    ata_init();

    vga_putstring("ATA driver Initialized\n");
    if (pmm_init(mbi) != PMM_SUCCESS)
    {
        vga_printf("failed pmm init");
        return;
    }

    if (!heap_init())
    {
        vga_printf("failed heap init");
        return;
    }

    pit_init();

    fat_init();

    keyboard_init();
    syscall_init();

    proc_manager_init();
    set_active_terminal(create_terminal(1));

    system_startup_animation();

    vga_init();
    //print_logo();

    create_kernelmode_process("/dummy", 0);
    create_usermode_process("/main", 0);

    asm("sti");
    enable_processes();

    while (1)
    {
    }
}


void simple_delay(volatile uint32_t count) {
    while(count--) {
        __asm__("nop");
    }
}

void system_startup_animation() {
    // Clear screen or prepare for startup sequence
    for (int i = 0; i < DEFAULT_VGA_HEIGHT * DEFAULT_VGA_WIDTH; i++) {
        vga_putchar_colored(' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
    }
    move_cursor(0, 0);

    // Expanded startup sequence with varied messages
    const char* startup_msgs[] = {
        "BOOTING SYSTEM",
        "INITIALIZING CORE MODULES",
        "LOADING KERNEL",
        "CHECKING HARDWARE INTEGRITY",
        "PREPARING MEMORY MANAGEMENT",
        "STARTING INTERRUPT HANDLERS",
        "CONFIGURING DEVICE DRIVERS",
        "INITIALIZING STORAGE SYSTEMS",
        "ESTABLISHING NETWORK PROTOCOLS",
        "PREPARING FILE SYSTEMS",
        "LOADING SYSTEM CACHE",
        "VERIFYING SYSTEM SIGNATURES",
        "INITIALIZING SECURITY MODULES",
        "PREPARING RUNTIME ENVIRONMENT",
        "LOADING SYSTEM LIBRARIES",
        "CONFIGURING USER PERMISSIONS",
        "CHECKING SYSTEM CLOCK",
        "INITIALIZING GRAPHICS SUBSYSTEM",
        "LOADING INPUT DEVICE DRIVERS",
        "PREPARING POWER MANAGEMENT",
        "FINAL SYSTEM CHECKS",
        "SYSTEM READY"
    };

    // Rapid color cycling startup animation
    for (int cycle = 0; cycle < 150; cycle++) {
        for (int msg_idx = 0; msg_idx < sizeof(startup_msgs)/sizeof(startup_msgs[0]); msg_idx++) {
            // Alternate and randomize background and text colors
            enum VGA_COLOR bg_colors[] = {
                VGA_COLOR_GREEN, VGA_COLOR_BLUE, VGA_COLOR_CYAN, 
                VGA_COLOR_MAGENTA, VGA_COLOR_LIGHT_BLUE, VGA_COLOR_LIGHT_GREEN
            };
            enum VGA_COLOR text_colors[] = {
                VGA_COLOR_WHITE, VGA_COLOR_YELLOW, VGA_COLOR_LIGHT_GREY, 
                VGA_COLOR_LIGHT_RED, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_PINK
            };

            // Select colors based on cycle to create variety
            enum VGA_COLOR bg_color = bg_colors[(cycle + msg_idx) % (sizeof(bg_colors)/sizeof(bg_colors[0]))];
            enum VGA_COLOR text_color = text_colors[(msg_idx + cycle) % (sizeof(text_colors)/sizeof(text_colors[0]))];

            // Print the message with selected colors
            vga_putstring_colored(
                (unsigned char*)startup_msgs[msg_idx], 
                bg_color,
                text_color
            );
            
            // Add variable number of random filler characters
            for (int j = 0; j < (cycle % 15); j++) {
                vga_putchar_colored(
                    // Alternate between space, dots, and dashes
                    (j % 3 == 0) ? ' ' : ((j % 3 == 1) ? '.' : '-'),
                    bg_colors[(j + cycle) % (sizeof(bg_colors)/sizeof(bg_colors[0]))], 
                    text_colors[(j + msg_idx) % (sizeof(text_colors)/sizeof(text_colors[0]))]
                );
            }
            
            // Varied delay to create more dynamic effect
            simple_delay(5000 * (cycle + msg_idx + 1));
        }
    }

    // Final clear screen or transition
    for (int i = 0; i < DEFAULT_VGA_HEIGHT * DEFAULT_VGA_WIDTH; i++) {
        vga_putchar_colored(' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
    }
    move_cursor(0, 0);
}