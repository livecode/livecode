#include "SkTypes.h"
#include <stdio.h>
#include <stdlib.h>

void sk_throw() {
    SkASSERT(!"sk_throw");
    abort();
}

void sk_out_of_memory(void) {
    SkASSERT(!"sk_out_of_memory");
    abort();
}

void* sk_malloc_throw(size_t size) {
    return sk_malloc_flags(size, SK_MALLOC_THROW);
}

void* sk_realloc_throw(void* addr, size_t size) {
    void* p = realloc(addr, size);
    if (size == 0) {
        return p;
    }
    if (p == NULL) {
        sk_throw();
    }
    return p;
}

void sk_free(void* p) {
    if (p) {
        free(p);
    }
}

void* sk_malloc_flags(size_t size, unsigned flags) {
    void* p = malloc(size);
    if (p == NULL) {
        if (flags & SK_MALLOC_THROW) {
            sk_throw();
        }
    }
    return p;
}

