#ifndef ALIGNEDMEM_H
#define ALIGNEDMEM_H
namespace FunAsr {
    extern void *aligned_malloc(size_t alignment, size_t required_bytes);
    extern void aligned_free(void *p);
}
#endif
