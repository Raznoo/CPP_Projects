#include "kernel.hh"
#include "k-apic.hh"
#include "k-vmiter.hh"
#include <atomic>

// kernel.cc
//
//    This is the kernel.


// INITIAL PHYSICAL MEMORY LAYOUT
//
//  +-------------- Base Memory --------------+
//  v                                         v
// +-----+--------------------+----------------+--------------------+---------/
// |     | Kernel      Kernel |       :    I/O | App 1        App 1 | App 2
// |     | Code + Data  Stack |  ...  : Memory | Code + Data  Stack | Code ...
// +-----+--------------------+----------------+--------------------+---------/
// 0  0x40000              0x80000 0xA0000 0x100000             0x140000
//                                             ^
//                                             | \___ PROC_SIZE ___/
//                                      PROC_START_ADDR

#define PROC_SIZE 0x40000       // initial state only

proc ptable[NPROC];             // array of process descriptors
                                // Note that `ptable[0]` is never used.
proc* current;                  // pointer to currently executing proc

#define HZ 100                  // timer interrupt frequency (interrupts/sec)
static std::atomic<unsigned long> ticks; // # timer interrupts so far


// Memory state
//    Information about physical page with address `pa` is stored in
//    `pages[pa / PAGESIZE]`. In the handout code, each `pages` entry
//    holds an `refcount` member, which is 0 for free pages.
//    You can change this as you see fit.

pageinfo pages[NPAGES];


[[noreturn]] void schedule();
[[noreturn]] void run(proc* p);
void exception(regstate* regs);
uintptr_t syscall(regstate* regs);
void memshow();


// kernel(command)
//    Initialize the hardware and processes and start running. The `command`
//    string is an optional string passed from the boot loader.

static void process_setup(pid_t pid, const char* program_name);

void kernel(const char* command) {
    // Initialize hardware.
    init_hardware();
    log_printf("Starting WeensyOS\n");

    // Initialize timer interrupt.
    ticks = 1;
    init_timer(HZ);

    // Clear screen.
    console_clear();

    // (re-)Initialize the kernel page table.
    for (vmiter it(kernel_pagetable); it.va() < MEMSIZE_VIRTUAL; it += PAGESIZE) {
        if (it.va() == 0 ) {//zero space
            it.map(it.va(), 0);
        } else if (it.va() == CONSOLE_ADDR ||it.va() > PROC_START_ADDR) {//console space & user space
            it.map(it.va(), PTE_P | PTE_W | PTE_U);
        } else {//kernel space
            it.map(it.va(), PTE_P | PTE_W);
        }
    }

    // Set up process descriptors.
    for (pid_t i = 0; i < NPROC; i++) {
        ptable[i].pid = i;
        ptable[i].state = P_FREE;
    }
    if (command && program_loader(command).present()) {
        process_setup(1, command);
    } else {
        process_setup(1, "allocator");
        process_setup(2, "allocator2");
        process_setup(3, "allocator3");
        process_setup(4, "allocator4");
    }

    // Switch to the first process using run().
    run(&ptable[1]);
}


// kalloc(sz)
//    Kernel memory allocator. Allocates `sz` contiguous bytes and
//    returns a pointer to the allocated memory (the physical address of
//    the newly allocated memory), or `nullptr` on failure.
//
//    The returned memory is initialized to 0xCC, which corresponds to
//    the x86 instruction `int3` (this may help you debug). You can
//    reset it to something more useful.
//
//    On WeensyOS, `kalloc` is a page-based allocator: if `sz > PAGESIZE`
//    the allocation fails; if `sz < PAGESIZE` it allocates a whole page
//    anyway.
//
//    The stencil code returns the next allocatable free page it can find,
//    but it never reuses pages or supports freeing memory (you'll have to
//    change this at some point).

static uintptr_t next_alloc_pa;

void* kalloc(size_t sz) {
//    log_printf("entering kalloc\n");
    if (sz > PAGESIZE) {
        return nullptr;
    }
    next_alloc_pa = 0;
    while (next_alloc_pa < MEMSIZE_PHYSICAL) {
        uintptr_t pa = next_alloc_pa;
        next_alloc_pa += PAGESIZE;

        if (allocatable_physical_address(pa)
            && !pages[pa / PAGESIZE].used()) {
            pages[pa / PAGESIZE].refcount += 1;
            memset((void*) pa, 0xCC, PAGESIZE);
//            log_printf("exiting kalloc\n");
            return (void*) pa;
        }
    }
    return nullptr;
}


// kfree(kptr)
//    Frees `kptr`, which must have been previously returned by `kalloc`.
//    If `kptr == nullptr` does nothing.

void kfree(void* kptr) {
//    log_printf("kfree\n");
    // Placeholder code below - you will have to implement `kfree`!

    if (kptr == nullptr || !pages[(uintptr_t)kptr/ PAGESIZE].used()){
        //do nothing
    } else {
        pages[(uintptr_t)kptr/ PAGESIZE].refcount -= 1;
    }
//    log_printf("leaving kfree\n");
}


// process_setup(pid, program_name)
//    Loads application program `program_name` as process number `pid`.
//    This loads the application's code and data into memory, sets its
//    %rip and %rsp, gives it a stack page, and marks it as runnable.

void process_setup(pid_t pid, const char* program_name) {
//    log_printf("process setup\n");
    bool procFail = false;
    init_process(&ptable[pid], 0);

    x86_64_pagetable* page_table_ptr = (x86_64_pagetable*)kalloc(sizeof(x86_64_pagetable));//makes page table
    if (page_table_ptr == nullptr){
        procFail = true;
    } else {
        memset((void*)page_table_ptr, 0x00 , sizeof(x86_64_pagetable));//sets contents of page table to zero character
        ptable[pid].pagetable = page_table_ptr;
    }

    // Initialize this process's page table.

    for (vmiter it(kernel_pagetable), it2(ptable[pid].pagetable);//creates an iterator for kernel PT and proc PT
    it.va() < PROC_START_ADDR; //will iterate up to size of the thing everyone shares
    it += PAGESIZE, it2 += PAGESIZE){//iterates both by the size of a page
            it2.map(it.pa(), it.perm());
            // nullptr is inaccessible even to the kernel
        }
    // Initialize `program_loader`.
    // The `program_loader` is an iterator that visits segments of executables.
    program_loader loader(program_name);

    // Using the loader, we're going to start loading segments of the program binary into memory
    // (recall that an executable has code/text segment, data segment, etc).

    // First, for each segment of the program, we allocate page(s) of memory.
    for (loader.reset(); loader.present() && !procFail; ++loader) {
        for (uintptr_t a = round_down(loader.va(), PAGESIZE);
             a < loader.va() + loader.size();
             a += PAGESIZE) {
            // `a` is the virtual address of the current segment's page.
            // Read the description on the `pages` array if you're confused about what it is.
            // Here, we're directly getting the page that has the same physical address as the
            // virtual address `a`, and claiming that page by incrementing its reference count
            void* pointer = kalloc(PAGESIZE);
            if (pointer == nullptr){
                break;
            } else {
                if (vmiter(ptable[pid].pagetable, a).try_map(pointer, PTE_P | loader.writable() * PTE_W | PTE_U) == -1){
                    kfree(pointer);
                    procFail = true;
                    break;
                }
            }
        }
    }
    if (!procFail){
        // We now copy instructions and data into memory that we just allocated.
        for (loader.reset(); loader.present(); ++loader) {
            vmiter it(ptable[pid].pagetable, loader.va());
            memset((void*) it.pa(), 0, loader.size());
            memcpy((void*) it.pa(), loader.data(), loader.data_size());
        }

        // Set %rip and mark the entry point of the code.
        ptable[pid].regs.reg_rip = loader.entry();

        // We also need to allocate a page for the stack.
        uintptr_t stack_addr = MEMSIZE_VIRTUAL- PAGESIZE;
        // Again, we're using the physical page that has the same address as the `stack_addr` to
        // maintain the one-to-one mapping between physical and virtual memory (you will have to change
        // this later).
        void* pointer = kalloc(PAGESIZE);
        if (pointer != nullptr){
            if (vmiter(ptable[pid].pagetable, stack_addr).try_map(pointer, PTE_P | PTE_W | PTE_U) == -1){
                kfree(pointer);
            }
        }

        // Set %rsp to the start of the stack.
        ptable[pid].regs.reg_rsp = stack_addr + PAGESIZE;

        // Finally, mark the process as runnable.
        ptable[pid].state = P_RUNNABLE;
//        log_printf("leaving proc setup\n");
    } else {
        for (vmiter it(ptable[current->pid].pagetable); it.va()<MEMSIZE_VIRTUAL; it += PAGESIZE){
            if (it.user() && it.va() != CONSOLE_ADDR){
                kfree((void*)(it.pa()));
            }
        }
        for (ptiter it(ptable[current->pid].pagetable); it.active(); it.next()) {
            kfree(it.kptr());
        }
        kfree(ptable[current->pid].pagetable);
//        log_printf("failed proc setup\n");

    }
}



// exception(regs)
//    Exception handler (for interrupts, traps, and faults).
//    You should *not* have to edit this function.
//
//    The register values from exception time are stored in `regs`.
//    The processor responds to an exception by saving application state on
//    the kernel's stack, then jumping to kernel assembly code (see
//    k-exception.S). That code saves more registers on the kernel's stack,
//    then calls exception(). This way, the process can be resumed right where
//    it left off before the exception. The pushed registers are popped and
//    restored before returning to the process (see k-exception.S).
//
//    Note that hardware interrupts are disabled when the kernel is running.

void exception(regstate* regs) {
    // Copy the saved registers into the `current` process descriptor.
    current->regs = *regs;
    regs = &current->regs;

    // It can be useful to log events using `log_printf`.
    // Events logged this way are stored in the host's `log.txt` file.
    /* log_printf("proc %d: exception %d at rip %p\n",
                current->pid, regs->reg_intno, regs->reg_rip); */

    // Show the current cursor location and memory state (unless this is a kernel fault).
    console_show_cursor(cursorpos);
    if (regs->reg_intno != INT_PF || (regs->reg_errcode & PFERR_USER)) {
        memshow();
    }

    // If Control-C was typed, exit the virtual machine.
    check_keyboard();


    // Actually handle the exception.
    switch (regs->reg_intno) {

    case INT_IRQ + IRQ_TIMER:
        ++ticks;
        lapicstate::get().ack();
        schedule();
        break;                  /* will not be reached */

    case INT_PF: {
        // Analyze faulting address and access type.
        uintptr_t addr = rdcr2();
        const char* operation = regs->reg_errcode & PFERR_WRITE
                ? "write" : "read";
        const char* problem = regs->reg_errcode & PFERR_PRESENT
                ? "protection problem" : "missing page";

        if (!(regs->reg_errcode & PFERR_USER)) {
            panic("Kernel page fault for %p (%s %s, rip=%p)!\n",
                  addr, operation, problem, regs->reg_rip);
        }
        console_printf(CPOS(24, 0), 0x0C00,
                       "Process %d page fault for %p (%s %s, rip=%p)!\n",
                       current->pid, addr, operation, problem, regs->reg_rip);
        current->state = P_BROKEN;
        break;
    }

    default:
        panic("Unexpected exception %d!\n", regs->reg_intno);

    }

    // Return to the current process (or run something else).
    if (current->state == P_RUNNABLE) {
        run(current);
    } else {
        schedule();
    }
}


// syscall(regs)
//    System call handler.
//
//    The register values from system call time are stored in `regs`.
//    The return value, if any, is returned to the user process in `%rax`.
//
//    Note that hardware interrupts are disabled when the kernel is running.

// Headers for helper functions used by syscall.
int syscall_page_alloc(uintptr_t addr);
pid_t syscall_fork();
void syscall_exit();

uintptr_t syscall(regstate* regs) {
    // Copy the saved registers into the `current` process descriptor.
    current->regs = *regs;
    regs = &current->regs;

    // It can be useful to log events using `log_printf`.
    // Events logged this way are stored in the host's `log.txt` file.
    /* log_printf("proc %d: syscall %d at rip %p\n",
                  current->pid, regs->reg_rax, regs->reg_rip); */

    // Show the current cursor location and memory state (unless this is a kernel fault).
    console_show_cursor(cursorpos);
    memshow();

    // If Control-C was typed, exit the virtual machine.
    check_keyboard();

    // Actually handle the exception.
    switch (regs->reg_rax) {

    case SYSCALL_PANIC:
        panic(nullptr); // does not return

    case SYSCALL_GETPID:
        return current->pid;

    case SYSCALL_YIELD:
        current->regs.reg_rax = 0;
        schedule(); // does not return

    case SYSCALL_PAGE_ALLOC:
        return syscall_page_alloc(current->regs.reg_rdi);

    case SYSCALL_FORK:
        return syscall_fork();

    case SYSCALL_EXIT:
        syscall_exit();
        schedule(); // does not return

    default:
        panic("Unexpected system call %ld!\n", regs->reg_rax);

    }

    panic("Should not get here!\n");
}


// syscall_page_alloc(addr)
//    Helper function that handles the SYSCALL_PAGE_ALLOC system call.
//    This function implement the specification for `sys_page_alloc`
//    in `u-lib.hh` (but in the stencil code, it does not - you will
//    have to change this).

int syscall_page_alloc(uintptr_t addr) {
    log_printf("page alloc\n");
    if (addr < PROC_START_ADDR || addr >= MEMSIZE_VIRTUAL) {
        return -1;
    } else{
        void* pointer = kalloc(PAGESIZE);
        if (pointer == nullptr){
            return -1;
        }
        if (vmiter(current->pagetable, addr).try_map(pointer, PTE_PWU) == -1){
            kfree(pointer);
            return -1;
        }
        log_printf("leaving page alloc\n");
        return 0;
    }
}

// syscall_fork()
//    Handles the SYSCALL_FORK system call. This function
//    implements the specification for `sys_fork` in `u-lib.hh`.
pid_t syscall_fork() {
//    log_printf("entering fork\n");
    // Implement for Step 5!
    int ptableIndex = 1;
    while (ptableIndex < NPROC){
        if (ptable[ptableIndex].state == P_FREE){
            break;
        } else {
            ptableIndex++;
        }
    }
    if (ptableIndex >= NPROC || ptableIndex == 0){
        return -1;
    } else {
//        log_printf("found ptable space, entering alloc\n");
        x86_64_pagetable* page_table_ptr = (x86_64_pagetable*)kalloc(sizeof(x86_64_pagetable));//makes page table
        if (page_table_ptr == nullptr){
//            log_printf("failed pagetable alloc\n");
            return -1;
        } else {
//            log_printf("succeeded in page table alloc\n");
            memset((void*)page_table_ptr, 0x00 , sizeof(x86_64_pagetable));//sets contents of page table to zero character
            ptable[ptableIndex].pagetable = page_table_ptr;
        }

        bool cleanupBool = false;
//        log_printf("entering page table copy\n");
        for (vmiter it(current->pagetable), it2(ptable[ptableIndex].pagetable);
             it.va() < MEMSIZE_VIRTUAL && !cleanupBool;
             it += PAGESIZE, it2 += PAGESIZE) {//iterates both by the size of a page
            //for shared space
            if (it.present() && (!it.writable()) && it.user() && it.va() != CONSOLE_ADDR){
//                log_printf("copying shared space\n");
                if (vmiter(ptable[ptableIndex].pagetable, it.va()).try_map(it.pa(), it.perm()) == -1){
//                    log_printf("failed copying shared space\n");
                    cleanupBool = true;
                } else {
//                    log_printf("succeeded copying shared space\n");
                    pages[(uintptr_t)it.pa()/PAGESIZE].refcount += 1;
                }
                //for sharing current process progress (heap & stack)
            } else if (it.present() && it.user() && it.va() != CONSOLE_ADDR){
//                log_printf("creating process user space\n");
                void* pointer = kalloc(PAGESIZE);
                if (pointer == nullptr){
//                    log_printf("failed allocating process user space\n");
                    cleanupBool = true;
                } else {
//                    log_printf("succeeded allocating process user space\n");
                    memcpy(pointer, (void*)it.pa(), PAGESIZE);
                    if (vmiter(ptable[ptableIndex].pagetable, it.va()).try_map(pointer, it.perm()) == -1){
//                        log_printf("failed to create mapping\n");
                        cleanupBool = true;
                        kfree(pointer);
                    }
                }
            } else {
//                log_printf("copying non user space and console\n");
                if (it2.try_map(it.pa(), it.perm()) == -1){
//                    log_printf("failed to create mapping\n");
                    cleanupBool = true;
                }
            }
            // nullptr is inaccessible even to the kernel
        }
        if (!cleanupBool){
            ptable[ptableIndex].pid = ptableIndex;
            ptable[ptableIndex].regs = current->regs;
            ptable[ptableIndex].regs.reg_rax = 0;
            ptable[ptableIndex].state = P_RUNNABLE;
            return ptableIndex;
        } else {
            for (vmiter it(ptable[ptableIndex].pagetable); it.va()<MEMSIZE_VIRTUAL; it += PAGESIZE){
                if (it.present() && it.user() && it.va() != CONSOLE_ADDR){
                    kfree((void*)(it.pa()));
                }
            }
            for (ptiter it(ptable[ptableIndex].pagetable); it.active(); it.next()) {
                kfree(it.kptr());
            }
            kfree(ptable[ptableIndex].pagetable);
            return -1;
        }

    }
}

// syscall_exit()
//    Handles the SYSCALL_EXIT system call. This function
//    implements the specification for `sys_exit` in `u-lib.hh`.
void syscall_exit() {
//    log_printf("entering exit\n");
    // Implement for Step 7!
    int pid = current->pid;
    for (vmiter it(ptable[pid].pagetable); it.va()<MEMSIZE_VIRTUAL; it += PAGESIZE){
        if ( it.present() && it.user() && it.va() != CONSOLE_ADDR){
            kfree((void*)(it.pa()));
        }
    }
    for (ptiter it(ptable[pid].pagetable); it.active(); it.next()) {
        kfree(it.kptr());
    }
    kfree(ptable[pid].pagetable);
    ptable[pid].state = P_FREE;
}

// schedule
//    Picks the next process to run and then run it.
//    If there are no runnable processes, spins forever.
//    You should *not* have to edit this function.

void schedule() {
    pid_t pid = current->pid;
    for (unsigned spins = 1; true; ++spins) {
        pid = (pid + 1) % NPROC;
        if (ptable[pid].state == P_RUNNABLE) {
            run(&ptable[pid]);
        }

        // If Control-C was typed, exit the virtual machine.
        check_keyboard();

        // If spinning forever, show the memviewer.
        if (spins % (1 << 12) == 0) {
            memshow();
            log_printf("%u\n", spins);
        }
    }
}


// run(p)
//    Runs process `p`. This involves setting `current = p` and calling
//    `exception_return` to restore its page table and registers.
//    You should *not* have to edit this function.

void run(proc* p) {
    assert(p->state == P_RUNNABLE);
    current = p;

    // Check the process's current pagetable.
    check_pagetable(p->pagetable);

    // This function is defined in k-exception.S. It restores the process's
    // registers then jumps back to user mode.
    exception_return(p);

    // should never get here
    while (true) {
    }
}


// memshow()
//    Draws a picture of memory (physical and virtual) on the CGA console.
//    Switches to a new process's virtual memory map every 0.25 sec.
//    Uses `console_memviewer()`, a function defined in `k-memviewer.cc`.
//    You should *not* have to edit this function.

void memshow() {
    static unsigned last_ticks = 0;
    static int showing = 0;

    // switch to a new process every 0.25 sec
    if (last_ticks == 0 || ticks - last_ticks >= HZ / 2) {
        last_ticks = ticks;
        showing = (showing + 1) % NPROC;
    }

    proc* p = nullptr;
    for (int search = 0; !p && search < NPROC; ++search) {
        if (ptable[showing].state != P_FREE
            && ptable[showing].pagetable) {
            p = &ptable[showing];
        } else {
            showing = (showing + 1) % NPROC;
        }
    }

    extern void console_memviewer(proc* vmp);
    console_memviewer(p);
}
