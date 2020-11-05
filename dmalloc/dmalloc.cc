#define DMALLOC_DISABLE 1
#include "dmalloc.hh"
#include <cassert>
#include <cstring>
#include <limits.h>
#include <vector>


dmalloc_stats globalStats;
int failSize_overflow;
std::vector<mallocPtrs> mallocPointers;//sorting is the devil
std::vector<void*> freedMallocPointers;
std::vector<mallocPtrs> historicMallocPointers;

char haxDetection[12] = "01234567200";


/**
 * dmalloc(sz,file,line)
 *      malloc() wrapper. Dynamically allocate the requested amount `sz` of memory and
 *      return a pointer to it
 *
 * @arg size_t sz : the amount of memory requested
 * @arg const char *file : a string containing the filename from which dmalloc was called
 * @arg long line : the line number from which dmalloc was called
 *
 * @return a pointer to the heap where the memory was reserved
 */
void* dmalloc(size_t sz, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    //bad switches
    bool overflowSwitch = false;

    if (sizeof(metadata) + sizeof(char[12]) > sizeof(metadata) + sizeof(char[12]) + sz){
        overflowSwitch = true;
    }
    //run through fails
    if(overflowSwitch){
        globalStats.nfail++;
        globalStats.fail_size += (unsigned long long)sz;
        return nullptr;
    } else {//otherwise execute
        void* myPtr = base_malloc(
                sizeof(metadata) + sz + sizeof(char[12]));//myPtr is the pointer to the beginning of metadata
        if (myPtr == nullptr) {
            //nfail
            globalStats.nfail++;
            //fail_size
            globalStats.fail_size += (unsigned long long)sz;
            return myPtr;
        } else {
            //nactive
            globalStats.nactive++;//increment everytime malloc is called
            //active_size
            globalStats.active_size += (unsigned long long)sz;//add size to phat stack
            //ntotal
            globalStats.ntotal++;
            //total_size
            globalStats.total_size += (unsigned long long)sz;
            metadata *metaPtr = (metadata *) myPtr;
            metaPtr->payloadSize = sz;
            metaPtr->haxCheck = *haxDetection;
            *((char*)myPtr + sizeof(metadata) + sz) = *haxDetection;
            void* endOfAlloc = (char*)myPtr + sizeof(metadata) + sz + sizeof(char[12]);
            //storing ptrs
            mallocPtrs newHistory;
            newHistory.start = myPtr;
            newHistory.fakePtr = (char*)myPtr + sizeof(metadata);
            newHistory.end = endOfAlloc;
            newHistory.file = (char*)file;
            newHistory.size = sz;
            newHistory.line = line;

            mallocPointers.push_back(newHistory);
            historicMallocPointers.push_back(newHistory);


            //heapmin & heapmax
            if (globalStats.heap_min > (uintptr_t)metaPtr|| globalStats.heap_min == 0){
                globalStats.heap_min = (uintptr_t)metaPtr;
            }
            if (globalStats.heap_max < (uintptr_t)endOfAlloc)
                globalStats.heap_max = (uintptr_t)endOfAlloc;
            void* returnThis = (char*)myPtr + sizeof(metadata);

            return returnThis;//this ptr is the user's
        }
    }
}

/**
 * dfree(ptr, file, line)
 *      free() wrapper. Release the block of heap memory pointed to by `ptr`. This should
 *      be a pointer that was previously allocated on the heap. If `ptr` is a nullptr do nothing.
 *
 * @arg void *ptr : a pointer to the heap
 * @arg const char *file : a string containing the filename from which dfree was called
 * @arg long line : the line number from which dfree was called
 */
void dfree(void* ptr, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    if (ptr != nullptr) {
        //good switches
        bool isIn = false;
        //start and end of allocated segment
        //start
        void* myPtr = (char *) ptr - sizeof(metadata);//move the pointer to real pointer

        for (int i = 0; i < (int)mallocPointers.size(); i++){//!check the thing to be deleted has been allocated
            if (mallocPointers[i].start == myPtr){
                isIn = true;
                freedMallocPointers.push_back(mallocPointers[i].start);
                mallocPointers.erase(mallocPointers.begin()+i);
                break;
            }
        }

        if (!isIn) {//!double frees check and bad pointer check
            for (int i = 0; i < (int) freedMallocPointers.size(); i++) {
                if (freedMallocPointers[i] == myPtr) {
                    fprintf(stderr, "MEMORY BUG: %s:%li: invalid free of pointer %p, double free",file, line, ptr);
                    abort();
                }
            }
            for (int i = 0; i < (int) mallocPointers.size(); i++) {
                if (mallocPointers[i].start < myPtr && myPtr < mallocPointers[i].end) {
                    fprintf(stderr, "MEMORY BUG: %s:%li: invalid free of pointer %p, not allocated\n",file, line, ptr);
                    fprintf(stderr, "%s:%lu: %p is %zu bytes inside a %zu byte region allocated here\n",
                            mallocPointers[i].file,
                            mallocPointers[i].line,
                            mallocPointers[i].fakePtr,
                            (size_t)ptr - (size_t)mallocPointers[i].fakePtr,
                            mallocPointers[i].size);
                    abort();
                    }
                }
            fprintf(stderr, "MEMORY BUG: %s:%li: invalid free of pointer %p, not in heap",file, line, ptr);
            abort();
            }
        else {
            //payload size finder
            metadata *metaPtr = (metadata *) myPtr;
            size_t sizeOfPayload = metaPtr->payloadSize;
            char* haxCheck = (char*)ptr + (int)sizeOfPayload;
            if (metaPtr->haxCheck != *haxDetection || *haxCheck != *haxDetection){
                fprintf(stderr, "MEMORY BUG: %s:%li: detected wild write during free of pointer %p",file, line, ptr);
                abort();
            }
            //freed pointers added to vectors
            freedMallocPointers.push_back(myPtr);//start of alloc
            //nactive
            globalStats.nactive--;
            //active_size
            globalStats.active_size -= (unsigned long long) sizeOfPayload;//sub size in phat stack
            //ntotal
            //!shouldn't change
            //total_size
            //!shouldn't change
            //nfail
            //!shouldn't change
            //fail_size
            //!shouldn't change
            //heapmin & heapmax
            //!shouldn't change
            base_free(myPtr);
        }
    }
}

/**
 * dcalloc(nmemb, sz, file, line)
 *      calloc() wrapper. Dynamically allocate enough memory to store an array of `nmemb`
 *      number of elements with wach element being `sz` bytes. The memory should be initialized
 *      to zero
 *
 * @arg size_t nmemb : the number of items that space is requested for
 * @arg size_t sz : the size in bytes of the items that space is requested for
 * @arg const char *file : a string containing the filename from which dcalloc was called
 * @arg long line : the line number from which dcalloc was called
 *
 * @return a pointer to the heap where the memory was reserved
 */
void* dcalloc(size_t nmemb, size_t sz, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    size_t localSZ = 0;
    int overflowSwitch = false;
    if (nmemb == 0) {
        void* ptr = dmalloc(localSZ, file, line);
        if (ptr) {
            memset(ptr, 0, nmemb * sz);
        }
        return ptr;
    } else if (nmemb == 1){
        void* ptr = dmalloc(sz, file, line);
        if (ptr) {
            memset(ptr, 0, nmemb * sz);
        }
        return ptr;
    }
    else {
        if (nmemb <= sz) {//makes checks faster....
            for (size_t i = 0; i < nmemb; i++) {
                if (sizeof(metadata) + sizeof(char[12]) + localSZ >
                    sizeof(metadata) + sizeof(char[12]) + localSZ + sz) {
                    overflowSwitch = 1;
                    break;
                }
                localSZ += sz;
            }
        }else {
            for (size_t i = 0; i < sz; i++) {
                if (sizeof(metadata) + sizeof(char[12]) + localSZ >
                    sizeof(metadata) + sizeof(char[12]) + localSZ + nmemb) {
                    overflowSwitch = 1;
                    break;
                }
                localSZ += nmemb;
            }
        }
        if (overflowSwitch != 0){
            //nfail
            globalStats.nfail++;
            //fail_size
            size_t addThis = nmemb * sz;
            globalStats.fail_size +=  addThis;
            return nullptr;

        }else {
            void* ptr = dmalloc(nmemb * sz, file, line);
            if (ptr) {
                memset(ptr, 0, nmemb * sz);
            }
            return ptr;
        }
    }

}

/**
 * get_statistics(stats)
 *      fill a dmalloc_stats pointer with the current memory statistics
 *
 * @arg dmalloc_stats *stats : a pointer to the the dmalloc_stats struct we want to fill
 */
void get_statistics(dmalloc_stats* stats) {

    memset(stats, 0, sizeof(dmalloc_stats));//resets possible garbage

    stats->nactive = globalStats.nactive;
    stats->active_size = globalStats.active_size;
    stats->ntotal = globalStats.ntotal;
    stats->total_size = globalStats.total_size;
    stats->nfail = globalStats.nfail;
    stats->fail_size = globalStats.fail_size;
    stats->heap_max = globalStats.heap_max;
    stats->heap_min = globalStats.heap_min;
}

/**
 * print_statistics()
 *      print the current memory statistics to stdout       
 */
void print_statistics() {
    dmalloc_stats stats;
    get_statistics(&stats);

    printf("alloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("alloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}

/**  
 * print_leak_report()
 *      Print a report of all currently-active allocated blocks of dynamic
 *      memory.
 */
void print_leak_report() {
    for (size_t i = 0; i < mallocPointers.size(); i++){
        printf("LEAK CHECK: %s:%li: allocated object %p with size %zu\n",
                mallocPointers[i].file, mallocPointers[i].line, mallocPointers[i].fakePtr,
                mallocPointers[i].size);
    }
}
