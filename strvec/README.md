Strings and Vectors!
===============

1) Because the pointer arithmetic is done locally (inside the function not in main) an never returned, the function
will still return "hello". If, inside the main function we would have called hello++, the pointer would've been moved
to the right one and printer out "ello" when called. 

2) Ptr1, if incremented would point to 0x2. Ptr2, if incremented, would point to 0x5.

3) Stack is where local variables go to live. Heap is dynamically allocated memory. Stack is faster to use than the
heap and stores all of the local variables created by functions. These varaibles only persist in the stack for the
lifetime of the function. Heap is dynamically allocated memory that is created by the user. Heap memory will continue
persisting until it is explicitly called to be freed again by the user. You would use Stack memory if the variable is
only important during the time of the function. You would use heap if you were interested in some data being
manipulated inside of one or many functions but persisting after their lifetime.

1) An array is a fixed length, immutable data structure. An array is a data stucture that is laid out in a specified
 location and each element of an array is adjacent in memory with no padding. A vector is mutable data structure that can grow or shrink
as needed. Vectors also require data to be allocated for the vector_t struct section and the 
area that the v->array is pointing to. You might use an array to store long strings of data that you don't want changed 
like a credit card number.
You might use vectors to encrypt some data that you might want hidden like a more secure credit card number :)

2)
main is in the stack
vector_type is in static or global memory
fun_adjective is in the stack
int_vector is in the stack

Initially, fun_vector was an initialized char* with nothing but null in the memory (8 bytes) set aside for it.
once strncpy was used on it, the contents of the same 8 bytes of data not have "awesome\0" stored in them
Initially, int_vector was just an initialized vector_t with nothing but null in the memory (32 bytes) that was set aside for it.
Each of it's fields were initially null but after the initialize, there is dynamically allocated space for the vector
that is being initialized. The fields are then initialized as follows:
    v->array = the address of the malloced memory
    v->ele_size = 4
    v->length = 1
    v->capacity = according to my implementation it is 4
=======
Project 1: Strvec
=================

<!-- TODO: Fill this out.  -->
## Design Overview:
NONE
## Collaborators:
Robert Boudreaux
## Conceptual Questions:
None
## How long did it take to complete Strings?
5 hours
## How long did it take to complete Vectors?
16 hours

