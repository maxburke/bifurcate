#ifndef BIFURCATE_CORE_H
#define BIFURCATE_CORE_H

namespace bc
{
    uint64_t hash(const char *string);
    uint64_t hash(const char *begin, const char *end);

    void *temp_alloc(size_t size);
    void *temp_get_mark();
    void temp_free_to_mark(void *mark);

    bool mem_map_file(void **memory, size_t *size, const char *fileName);
    void mem_unmap_file(void *memory);

    class auto_mem_map
    {
        void *mMem;
        size_t mSize;
        const char *mFileName;

        auto_mem_map &operator =(const auto_mem_map &);
        auto_mem_map(const auto_mem_map &);

    public:
        auto_mem_map(const char *file_name);
        ~auto_mem_map();

        void *mem() { return mMem; }
        const void *mem() const { return mMem; }
        size_t size() const { return mSize; }
        bool valid() const { return mMem != NULL && mSize != 0; }
    };

    enum pool_t
    {
        POOL_INVALID,
        POOL_STRING,
        POOL_ANIM,
        POOL_GFX
    };

    void *_internal_mem_alloc(pool_t pool, size_t size, size_t align, int line, const char *file);
    #define mem_aligned_alloc(pool, size, align) _internal_mem_alloc(pool, size, align, __LINE__, __FILE__)
    #define mem_alloc(pool, size) _internal_mem_alloc(pool, size, 16, __LINE__, __FILE__)

    const char *intern(const char **out, uint64_t *out_hash, const char *begin, const char *end);
    const char *intern(const char **out, uint64_t *out_hash, const char *str);

    void mem_free(void *block);
}

#endif
