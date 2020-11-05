Project 3 - WeensyOS
====================

<!-- TODO: Fill this out. -->

## Design Overview:
None
## Collaborators:
 None
## Conceptual Questions:
   General Questions:
   1)There might be a security risk associated with a process knowing exactly where in physical memory it resides. Its
   much easier to execute some memory attack if the process knows exactly how it will interface with physical memory.
   The purpose of mapping physical addresses to different virtual memory addresses is to get rid of this identity
   mapping. If we create a unique mapping for each process, the memory it sees is still usable in the sense that
   a standard program uses memory, but doesn't include the major security risks associated with a process knowing
   exactly how it interacts with physical memory (bad for buffer overflow attacks) 
   2) The kernel is an extremely privileged section of an operating system because there needs to be a way to handle
   every possible case (within reason, most kernels probably don't handle demonic possession) of an operating system.
   Additionally, it is important that a piece of the operating system be able to provide other pieces (processes) with
   certain checks to make sure they are not behaving maliciously.
   As an example: If there was a process that was trying to alter contents of another process, that would change the
   expected behavior of another process into a possibly unpredictable path. This would, at the very least, make 
   debugging the program you think is behaving incorrectly more difficult to debug. This problem can be stopped with a
   kernel! The kernel will check and make sure that each operating system stays in the memory that has been allocated
   only for that process.
   As another example: a process that is not intentionally behaving maliciously might take a tremendous amount of 
   processing power to execute. In an operating system without a kernel(and with only one processor), each process will
   execute one after another and  if one program is stuck in an infinite loop, the operating system without a kernel
   will execute that loop forever until power is removed from the computer. An operating system with a kernel might be
   able to kill a process that is stuck in this loop and let other processes run!
   Project Specific Questions:
   1) A page table for a brand new process will have its "process" pieces (text, code, data, stack) physically allocated.
   After physical allocation, the process will need to map the new physical allocations to its virtual address space on
   the page table. for a forked process, this process will use the same read only memory segments of its parent process.
   This means that it undergoes the exact same process, but on the first two iterations of page table allocation, 
   instead of allocating additional physical pages, it will just map the new process's virtual address to its parent
   process's same physical address for that particular page.
   2)a process will refer to its physical memory by accessing its page table. A process acesses it's page table by
   passing a particular virtual address to a place in memory. Then the address passed in will traverse the page table
   and be converted to that virtual address's corresponding physical address.
   3)When a process exits, the kernel's "knowledge about the process also needs to be cleared". In WeensyOS this is
   done by setting the state to P_FREE and iterating through that process's current pagetable, following each one of
   the physical addresses contained in the pagetable and reducing the reference count of that physical page...
   with appropriate error checking along the way.
## How long did it take to complete WeensyOS?
part 1: 12 Hours
part 2: 16 Hours

<!-- Enter an approximate number of hours that you spent actively working on the project. -->
