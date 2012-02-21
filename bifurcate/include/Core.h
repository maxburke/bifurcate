#ifndef BIFURCATE_CORE_H
#define BIFURCATE_CORE_H

namespace bc
{
    uint64_t Hash(const char *string);
    uint64_t Hash(const char *begin, const char *end);

    class TempMemMarker
    {
        void *mPtr;

        TempMemMarker &operator =(const TempMemMarker &);
        TempMemMarker(const TempMemMarker &);

    public:
        TempMemMarker();
        ~TempMemMarker();

        void Reset();
    };

    void *TempAlloc(size_t size);

    bool MemMapFile(void **memory, size_t *size, const char *fileName);
    void MemUnmapFile(void *memory);

    class AutoMemMap
    {
        void *mMem;
        size_t mSize;
        const char *mFileName;

        AutoMemMap &operator =(const AutoMemMap &);
        AutoMemMap(const AutoMemMap &);

    public:
        AutoMemMap(const char *file_name);
        ~AutoMemMap();

        void *Mem() { return mMem; }
        const void *Mem() const { return mMem; }
        size_t Size() const { return mSize; }
        bool Valid() const { return mMem != NULL && mSize != 0; }
    };

    enum pool_t
    {
        POOL_INVALID,
        POOL_STRING,
        POOL_ANIM,
        POOL_GFX,
        POOL_MESH
    };

    void *_InternalMemAlloc(pool_t pool, size_t size, size_t align, int line, const char *file);
    #define MemAlignedAlloc(pool, size, align) bc::_InternalMemAlloc(pool, size, align, __LINE__, __FILE__)
    #define MemAlloc(pool, size) bc::_InternalMemAlloc(pool, size, 16, __LINE__, __FILE__)

    const char *Intern(const char **out, uint64_t *out_hash, const char *begin, const char *end);
    const char *Intern(const char **out, uint64_t *out_hash, const char *str);

    void mem_free(void *block);
}

#endif
