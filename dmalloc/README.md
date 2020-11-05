Project 2: DMalloc
===================

<!-- TODO: Fill this out. -->
## Design Overview:

## Collaborators:

## Conceptual Questions:
1) Static memory wouldn't be good to use in a case where you need to store a large amount of data or 
    data that needs to grow or shrink in size.
    Dynamic memory is better than static for storing large data as well as data that might need to grow or shrink in size
    an example of this is a large linked list where you erase or add nodes.
    Automatic memory wouldn't be good if you wanted to use memory outside of a function.
    Dynamic memory is allocated and can be manipulated as long as it hasn't been freed

2) Every call to malloc or realloc (heap growing) assigns a random part of memory within an address range
    as an example, if I malloc enough space for each node in a linked list, each node will be in a random spot on the heap
    with each reference to a node being returned as a pointer to that position in memory

Stack memory is allocated adjacent to the last stack allocaton. So the second function called will be adjacent to
    the previous function allocated to the stack
    Stack memory is allocated this way because of the standard calling convention! It is up to the compiler to be able to traverse a local funtion(stack memory) with very little extra processing power.
    one of the least expensive (and predictable) ways of doing that is by allocating memory for the functions adjacent to the previous allocation.
    It is also a requirement for the size of the stack to be known at compile time.

3) Struct is a user defined data structure. It is an efficient way of organizing a pattern of data defined by a user.
    If I wanted to make a tuple in C, there is no tuple datatype, instead, I would have to make a struct with a specified key and value type

4)  (char *) ptr + 1 = 0x12f99a65
    (int *) ptr + 1 = 0x12f99a68
    the reason why is because a pointer cast as a certain data type (x) responds to pointer arithmetic in multiples of the sizeOf(x). So in the above case, adding one to an integer pointer is the same as incrementing the pointer by 3

5) when memory is dynamically allocated but not freed. This is bad because the memory that was allocated is still accessible (by potentially malicious users). This could be a program that provides a credit checking service saving your SSN to dynamic memory.
    A memory leak of that data would allow a malicious user / program to access that memory and use it for their own nefarious purposes.

6) You could try to access memory that wasn't initialized
    You could not have enough memory allocated for the data you are trying to store.
    You could try to reference memory that has already been freed

7) Checking for integer overflow in multiplication probably should be done in a sort of loop. The check should be something along the lines of...
    add (A) to (A) an amount of times equivalent to (X) if x != 0 at each addition, check if it has overflowed. Where as a standard addition check
    would just preform the a similar action... add (A) to (B) 1 time and if there are enough bits to hold the new value then you are good! Multiplication
    overflow checks can also be shorted to zero by multiplying a number by 0. Multiplication checks also have to account for negative times positive being negative 
    (so switch the sign representation of the stored data)
    where as a standard addition check would just add a number to another number regardless of how positive or negative the two numbers are!
## How long did it take to complete DMalloc?
