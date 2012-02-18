#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <algorithm>
#pragma warning(pop)

#include <malloc.h>

#include "Config.h"
#include "Core.h"

namespace bc
{
    uint64_t hash(const char *string)
    {
        const uint64_t prime = 1099511628211ULL;
        uint64_t rv = 14695981039346656037ULL;
        for (const char *ptr = string; ; ++ptr)
        {
            const char c = *ptr;
            if (c == 0)
                break;

            rv = (rv ^ c) * prime;
        }

        return rv;
    }

    uint64_t hash(const char *begin, const char *end)
    {
        const uint64_t prime = 1099511628211ULL;
        uint64_t rv = 14695981039346656037ULL;
        for (const char *ptr = begin; ptr != end; ++ptr)
        {
            const char c = *ptr;
            rv = (rv ^ c) * prime;
        }

        return rv;
    }

    struct file_map_data
    {
        HANDLE mFileHandle;
        HANDLE mFileMapping;
        LPVOID mMemory;
    };

    const int MAX_NUM_MAPPED_FILES = 16;
    __declspec(thread) static file_map_data gfile_map_data[MAX_NUM_MAPPED_FILES];
    
    const size_t TEMP_HEAP_SIZE = 4 * 1024 * 1024;
    __declspec(thread) __declspec(align(16)) static unsigned char gTempHeap[TEMP_HEAP_SIZE];
    __declspec(thread) static unsigned char *gTempHeapPtr;

    void *temp_alloc(size_t size)
    {
        if (gTempHeapPtr == NULL)
            gTempHeapPtr = gTempHeap;

        const size_t alignedSize = (size + 15) & ~15;
        gTempHeapPtr = gTempHeapPtr + alignedSize;
        return gTempHeapPtr;
    }

    void *temp_get_mark()
    {
        if (gTempHeapPtr == NULL)
            gTempHeapPtr = gTempHeap;

        return gTempHeapPtr;
    }

    void temp_free_to_mark(void *mark)
    {
        assert(mark <= gTempHeapPtr);
        gTempHeapPtr = static_cast<unsigned char *>(mark);
    }

    bool mem_map_file(void **memory, size_t *size, const char *fileName)
    {
        HANDLE fileHandle = NULL;
        HANDLE fileMapping = NULL;
        int fileMapIndex;

        for (fileMapIndex = 0; fileMapIndex < MAX_NUM_MAPPED_FILES; ++fileMapIndex)
            if (gfile_map_data[fileMapIndex].mMemory == NULL)
                break;

        verify(fileMapIndex < MAX_NUM_MAPPED_FILES);

    #define VALIDATE(VALUE) if (!(VALUE)) { *memory = NULL; *size = 0; if (fileMapping) CloseHandle(fileMapping); if (fileHandle) CloseHandle(fileHandle); return false; } else ((void)0)
        fileHandle = CreateFile(fileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        VALIDATE(fileHandle);

        DWORD fileSizeLow = 0;
        DWORD fileSizeHigh = 0;
        fileSizeLow = GetFileSize(fileHandle, &fileSizeHigh);

        fileMapping = CreateFileMapping(fileHandle,
            NULL,
            PAGE_READONLY,
            0,
            0,
            NULL);

        VALIDATE(fileMapping);

        LPVOID mappedMemory = MapViewOfFile(fileMapping,
            FILE_MAP_READ,
            0,
            0,
            0);

        VALIDATE(mappedMemory);

        ULARGE_INTEGER fileSize;
        fileSize.LowPart = fileSizeLow;
        fileSize.HighPart = fileSizeHigh;
        *size = static_cast<size_t>(fileSize.QuadPart);
        *memory = mappedMemory;

        file_map_data currentMappingData = { fileHandle, fileMapping, mappedMemory };
        gfile_map_data[fileMapIndex] = currentMappingData;

        return true;
    #undef VALIDATE
    }

    void mem_unmap_file(void *memory)
    {
        verify(memory != NULL);

        int fileMapIndex;
        for (fileMapIndex = 0; fileMapIndex < MAX_NUM_MAPPED_FILES; ++fileMapIndex)
            if (gfile_map_data[fileMapIndex].mMemory == memory)
                break;

        verify(fileMapIndex < MAX_NUM_MAPPED_FILES);
        UnmapViewOfFile(memory);
        CloseHandle(gfile_map_data[fileMapIndex].mFileMapping);
        CloseHandle(gfile_map_data[fileMapIndex].mFileHandle);

        file_map_data mappingData = { NULL, NULL, NULL };
        gfile_map_data[fileMapIndex] = mappingData;
    }

    auto_mem_map::auto_mem_map(const char *file_name)
        : mMem(NULL),
          mSize(0),
          mFileName(file_name)
    {
        mem_map_file(&mMem, &mSize, mFileName);
    }

    auto_mem_map::~auto_mem_map()
    {
        mem_unmap_file(mMem);
    }

    void *_internal_mem_alloc(pool_t pool, size_t size, size_t align, int line, const char *file)
    {
        UNUSED(line);
        UNUSED(pool);
        UNUSED(file);
        return _aligned_malloc(size, align);
    }

    void mem_free(void *block)
    {
        _aligned_free(block);
    }

    struct intern_table_entry
    {
        uint64_t mHash;
        const char *mString;
    };

    struct intern_table
    {
        intern_table *mNext;
        char *mPtr;
        intern_table_entry *mInternTableEntry;
        intern_table_entry *mInternTableEntryEnd;
        char mBuffer[4096 - 4 * sizeof(void *)];
    };

    static intern_table *allocate_intern_table()
    {
        intern_table *table = static_cast<intern_table *>(VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        table->mNext = 0;
        table->mPtr = table->mBuffer;
        table->mInternTableEntryEnd = (intern_table_entry *)((char *)table + 4096);
        table->mInternTableEntry = table->mInternTableEntryEnd;

        return table;
    }

    static bool entry_comparer(const intern_table_entry &a, const intern_table_entry &b)
    {
        return a.mHash < b.mHash;
    }

    static const char *search_tables(intern_table *table, uint64_t hash)
    {
        if (table == NULL)
            return NULL;

        intern_table_entry target = { hash, 0 };

        std::pair<intern_table_entry *, intern_table_entry *> bounds = std::equal_range(
                table->mInternTableEntry,
                table->mInternTableEntryEnd,
                target, 
                entry_comparer);

        if (bounds.first < bounds.second)
            return bounds.first->mString;
        return search_tables(table->mNext, hash);
    }

    const char *intern(const char **out_ptr, uint64_t *out_hash, const char *begin, const char *end)
    {
        static_assert(sizeof(intern_table) == 4096, "Intern table entries must be page-sized");
        static intern_table *gInternTable;

        if (gInternTable == NULL)
            gInternTable = allocate_intern_table();

        uint64_t stringHash = hash(begin, end);
        const char *string = search_tables(gInternTable, stringHash);
        if (string)
            goto finish;

        intern_table *table = gInternTable;

        size_t length = static_cast<size_t>(begin - end);
        const size_t max_length = sizeof(table->mBuffer) - sizeof(intern_table_entry);
        assert(length < max_length);

        const size_t required_space = length + 1 + sizeof(intern_table_entry);
        const size_t available_space = reinterpret_cast<uintptr_t>(table->mInternTableEntry) 
            - reinterpret_cast<uintptr_t>(table->mPtr);
        if (required_space > available_space)
            table = allocate_intern_table();
        string = table->mPtr;

        memcpy(const_cast<char *>(string), begin, length);
        table->mPtr[length] = 0;
        table->mPtr += length + 1;

        intern_table_entry *entry = --table->mInternTableEntry;
        entry->mHash = stringHash;
        entry->mString = string;
        std::sort(table->mInternTableEntry, table->mInternTableEntryEnd, entry_comparer);

    finish:
        if (out_hash)
            *out_hash = stringHash;
        if (out_ptr)
            *out_ptr = string;

        return string;
    }

    const char *intern(const char **out, uint64_t *out_hash, const char *str)
    {
        return intern(out, out_hash, str, str + strlen(str));
    }
}
