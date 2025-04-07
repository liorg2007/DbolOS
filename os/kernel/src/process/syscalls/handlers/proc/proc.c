#include "proc.h"
#include "process/manager/process_manager.h"
#include "memory/paging/paging.h"
#include "memory/heap/heap.h"

#define ALIGN_UP(size, alignment) (((size) + (alignment)-1) & ~((alignment)-1))

void _exit(int status)
{
    exit_current_process();
}

int _execve(const char *pathname, char *const argv[], char *const envp[])
{
    int code = create_usermode_process(pathname, 0);
    force_switch_process();
    return code;
}

int _getpid()
{
    return get_current_process()->pid;
}

void* _sbrk(int increment)
{
    process_t* proc = get_current_process();
    uintptr_t proc_break = proc->process_break;
    uintptr_t current_page_end = ALIGN_UP(proc_break, PAGE_SIZE);
    uintptr_t page_tables_boundary = ALIGN_UP(proc_break, PAGES_PER_TABLE * PAGE_SIZE);

    if (get_physical_address((void*)proc->process_break) == 0)
    {
        return (void*)-1;
    }
    if (proc_break + increment > current_page_end)
    {
        page_table_entry* new_page_tables = NULL;
        int amount_page_tables_to_allocate = 0;
        int amount_pages_to_allocate_past_boundary = 0;

        if (proc_break + increment > page_tables_boundary)
        {
            // allocate the pages past the boundary with the new page tables
            amount_page_tables_to_allocate = (proc_break + increment - page_tables_boundary) 
                / (PAGES_PER_TABLE * PAGE_SIZE) + 1;
            amount_pages_to_allocate_past_boundary = (amount_page_tables_to_allocate - 1)
                * PAGES_PER_TABLE + 1;
            new_page_tables = kmalloc_pages(amount_page_tables_to_allocate);
            
            if (!new_page_tables || !allocate_multiple_pages(page_tables_boundary / PAGE_SIZE,
                amount_pages_to_allocate_past_boundary, false, get_physical_address(new_page_tables))) 
            {
                kfree(new_page_tables);
                return (void*)-1;
            }
        }

        // allocate the rest of the pages (before the boundary)
        int amount_pages_to_allocate = (proc_break + increment - current_page_end) / PAGE_SIZE
            - amount_pages_to_allocate_past_boundary;
        
        if (!allocate_multiple_pages(current_page_end / PAGE_SIZE,
            amount_pages_to_allocate, false, get_physical_address(get_page_table((void*)current_page_end))))
        {
            for (int i = 0; i < amount_page_tables_to_allocate; i++)
            {
                kfree(new_page_tables + i * PAGE_SIZE);
            }
            return (void*)-1;
        }
    }
    proc->process_break += increment;
    return (void*)proc_break;
}