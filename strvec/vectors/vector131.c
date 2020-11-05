#include "vector131.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Initializes all of the fields of a vector struct
 *Arguments:
 * - v: a pointer to a vector_t
 * - type: a size_t indicating the size of the type of this vector's elements
 *Return value: none
 */
void initialize_vector(vector_t* v, size_t type) {
  v->array = (void*)malloc(type);//this returns a pointer to the array field which will point to the memory set aside in the vector
  v->ele_size = type;
  v->length = 0;
  v->capacity = 1;
}

/*Frees this vector
 *Arguments:
 * - v: a pointer to the vector which needs to be freed
 *Return value: none
 */
void destroy_vector(vector_t* v) {
  free(v->array);
}

/*Gets the size of a given vector
 *Arguments:
 * - v: the vector whose size is desired
 *Return value: an integer containing the size of the vector
 */
size_t vector_size(vector_t* v) {
  return v->length;
}

/*Gets the element at a desired position within a vector
 *Arguments:
 * - v: a pointer to a vector_t
 * - index: the index of the desired element in v (with 0 indexing)
 *Return value: a void pointer to the element at index (to be casted
 *appropriately by the caller)
 */
void* vector_get(vector_t* v, int index) {
  return (char*)v->array + index * (int)v->ele_size;
}

/*Adds an element to the back of a vector, doubling the capacity of the vector
 *if needed Arguments:
 * - v: a pointer to a vector_t
 * - ele: a pointer to the element to be copied into v
 * Return value: none
 */
void vector_add_back(vector_t* v, void* ele) {
    size_t vectorSize = v->length * v->ele_size;// so that we compute vector size once
    if(v->capacity < v->length + 1){///fixed this
      v->array = realloc(v->array, v->capacity * v->ele_size * 2);
      memcpy(v->array + vectorSize, ele, v->ele_size);
      v->length += 1;
      v->capacity *= 2;///fixed this
    } else{
        memcpy((char*)v->array + (int)vectorSize, ele, v->ele_size);
        v->length = v->length +1;
    }
}

/*Removes the last element in a vector
 *Arguments:
 * - v: a pointer to a vector_t
 *Return value: none
 */
void vector_delete_back(vector_t* v) {
    v->length -= 1;
}

/*Adds an element to a specified index in a vector, double its capacity if
 *needed Arguments:
 * - v: a pointer to a vector_t
 * - ele: a pointer to the element to be copied into v
 * - index: the desired index for ele in the vector, v (using 0 indexing)
 *Return value: none
 */
void vector_add(vector_t* v, void* ele, int index) {
    size_t evictedMem = (v->length - (size_t)index) * v->ele_size;
    if(v->capacity < v->length + 1){
        v->array = realloc(v->array, v->capacity * v->ele_size * 2);
        void* goHere = (char*)v->array + (index * (int)v->ele_size);
        memmove((char*)goHere + (int)v->ele_size, goHere, evictedMem);
        memcpy(goHere, ele,  v->ele_size);
        v->length += 1;
        v->capacity *= 2;
    } else {
        void* goHere = (char*)v->array + (index * (int)v->ele_size);
        memmove((char*)goHere + (int)v->ele_size, goHere, evictedMem);
        memcpy(goHere, ele,  v->ele_size);
        v->length += 1;
    }
}

/*Deletes an element from the specified position in a vector
 *Arguments:
 * - v: a pointer to a vector_t
 * - index: the index of the element to be deleted from v (using 0 indexing)
 *Return value: none
 */
void vector_delete(vector_t* v, int index) {
    size_t evictedMem = (v->length - (size_t)index) * v->ele_size;
    void* moveHere = (char*)v->array + (index * (int)v->ele_size);
    memmove(moveHere, moveHere + v->ele_size, evictedMem);
    v->length -= 1;
}
