#include <sys/mman.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

typedef struct CodeBlock
{
    // Memory block
    uint8_t *mem_block;

    // Memory block size
    uint32_t mem_size;
} codeblock_t;

// Align the current write position to a multiple of bytes
static uint8_t *align_ptr(uint8_t *ptr, uint32_t multiple)
{
    // Compute the pointer modulo the given alignment boundary
    uint32_t rem = ((uint32_t)(uintptr_t)ptr) % multiple;

    // If the pointer is already aligned, stop
    if (rem == 0)
        return ptr;

    // Pad the pointer by the necessary amount to align it
    uint32_t pad = multiple - rem;

    return ptr + pad;
}

#define CB_MARK_WRITEABLE(cb) do { \
    if (mprotect(cb->mem_block, cb->mem_size, PROT_READ | PROT_WRITE)) { \
        fprintf(stderr, "Couldn't make JIT page (%p) writeable, errno: %s", (void *)cb->mem_block, strerror(errno)); \
        abort(); \
    } \
} while(0);

#define CB_MARK_EXECUTABLE(cb) do { \
    if (mprotect(cb->mem_block, cb->mem_size, PROT_READ | PROT_EXEC)) { \
        fprintf(stderr, "Couldn't make JIT page (%p) writeable, errno: %s", (void *)cb->mem_block, strerror(errno)); \
        abort(); \
    } \
} while(0);

uint8_t *alloc_exec_mem(uint32_t mem_size)
{
    uint8_t *mem_block;

    #if defined(MAP_FIXED_NOREPLACE) && defined(_SC_PAGESIZE)
        // Align the requested address to page size
        uint32_t page_size = (uint32_t)sysconf(_SC_PAGESIZE);
        uint8_t *req_addr = align_ptr((uint8_t*)&alloc_exec_mem, page_size);

        do {
            // Try to map a chunk of memory as executable
            mem_block = (uint8_t*)mmap(
                (void*)req_addr,
                mem_size,
                PROT_READ | PROT_EXEC,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                -1,
                0
            );

            // If we succeeded, stop
            if (mem_block != MAP_FAILED) {
                return mem_block;
            }

            // +4MB
            req_addr += 4 * 1024 * 1024;
        } while (req_addr < (uint8_t*)&alloc_exec_mem + INT32_MAX);

    // On MacOS and other platforms
    #else
        // Try to map a chunk of memory as executable
        mem_block = (uint8_t*)mmap(
            (void*)alloc_exec_mem,
            mem_size,
            PROT_READ | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0
        );
    #endif

    // Fallback
    if (mem_block == MAP_FAILED) {
        // Try again without the address hint (e.g., valgrind)
        mem_block = (uint8_t*)mmap(
            NULL,
            mem_size,
            PROT_READ | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0
            );
    }

    // Check that the memory mapping was successful
    if (mem_block == MAP_FAILED) {
        perror("mmap call failed");
        exit(-1);
    }

    return mem_block;
}

int main(int argc, char *argv[]) {
    uint32_t page_multiple = 0;
    uint32_t iterations = 0;
    uint8_t *mem_block;
    uint32_t page_size = (uint32_t)sysconf(_SC_PAGESIZE);

    if (getenv("PAGE_MULTIPLE")) {
        page_multiple = atoi(getenv("PAGE_MULTIPLE"));
    } else {
        fprintf(stderr, "Please supply PAGE_MULTIPLE\n");
        exit(-1);
    }

    if (getenv("ITERATIONS")) {
        iterations = atoi(getenv("ITERATIONS"));
    } else {
        fprintf(stderr, "Please supply ITERATIONS\n");
        exit(-1);
    }

    size_t mem_size = page_multiple * page_size;

    fprintf(stdout, "Allocating page size %zu\n", mem_size);
    fprintf(stdout, "Iterating %u times\n", iterations);

    codeblock_t code;
    code.mem_block = alloc_exec_mem(mem_size);
    code.mem_size = mem_size;
    codeblock_t *cb = &code;

    for (uint32_t i = 0; i < iterations; i++) {
        CB_MARK_WRITEABLE(cb);
        CB_MARK_EXECUTABLE(cb);
    }

    return 0;
}
