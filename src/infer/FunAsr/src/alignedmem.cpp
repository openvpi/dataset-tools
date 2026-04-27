#include <string>

namespace FunAsr {
    void *aligned_malloc(size_t alignment, size_t required_bytes) {
        void *p1; // original block
        const int offset = alignment - 1 + sizeof(void *);
        if ((p1 = (void *) malloc(required_bytes + offset)) == nullptr) {
            return nullptr;
        }
        // aligned block
        auto **p2 = (void **) (((size_t) (p1) + offset) & ~(alignment - 1));
        p2[-1] = p1;
        return p2;
    }

    void aligned_free(void *p) {
        free(((void **) p)[-1]);
    }
}