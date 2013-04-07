#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <algorithm>
#pragma warning(pop)

#include <malloc.h>
#include <stdarg.h>

#include "Config.h"
#include "Core.h"

namespace bc
{
    uint64_t Hash(const char *string)
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

    uint64_t Hash(const char *begin, const char *end)
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

    void *TempAlloc(size_t size)
    {
        if (gTempHeapPtr == NULL)
            gTempHeapPtr = gTempHeap;

        void *ptr = gTempHeapPtr;

        const size_t alignedSize = (size + 15) & ~15;
        gTempHeapPtr = gTempHeapPtr + alignedSize;

        return ptr;
    }

    TempMemMarker::TempMemMarker()
    {
        if (gTempHeapPtr == NULL)
            gTempHeapPtr = gTempHeap;

        mPtr = gTempHeapPtr;
    }

    TempMemMarker::~TempMemMarker()
    {
        assert(mPtr <= gTempHeapPtr);
        Reset();
    }

    void TempMemMarker::Reset()
    {
        gTempHeapPtr = static_cast<unsigned char *>(mPtr);
    }

    bool MemMapFile(void **memory, size_t *size, const char *fileName)
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

    void MemUnmapFile(void *memory)
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

    AutoMemMap::AutoMemMap(const char *file_name)
        : mMem(NULL),
          mSize(0),
          mFileName(file_name)
    {
        MemMapFile(&mMem, &mSize, mFileName);
    }

    AutoMemMap::~AutoMemMap()
    {
        MemUnmapFile(mMem);
    }

    Futex::Futex()
    {
        static_assert(sizeof(mFutexStorage) == sizeof(CRITICAL_SECTION), "Ensure that the futex storage is big enough");
        LPCRITICAL_SECTION criticalSection = (LPCRITICAL_SECTION)mFutexStorage;
        InitializeCriticalSection(criticalSection);
    }

    Futex::~Futex()
    {
        LPCRITICAL_SECTION criticalSection = (LPCRITICAL_SECTION)mFutexStorage;
        DeleteCriticalSection(criticalSection);
    }

    void Futex::Lock()
    {
        LPCRITICAL_SECTION criticalSection = (LPCRITICAL_SECTION)mFutexStorage;
        EnterCriticalSection(criticalSection);
    }

    bool Futex::TryLock()
    {
        LPCRITICAL_SECTION criticalSection = (LPCRITICAL_SECTION)mFutexStorage;
        return TryEnterCriticalSection(criticalSection) != 0;
    }

    void Futex::Unlock()
    {
        LPCRITICAL_SECTION criticalSection = (LPCRITICAL_SECTION)mFutexStorage;
        LeaveCriticalSection(criticalSection);
    }

    void *_InternalMemAlloc(int pool, size_t size, size_t align, int line, const char *file)
    {
        UNUSED(line);
        UNUSED(pool);
        UNUSED(file);
        return _aligned_malloc(size, align);
    }

    void MemFree(void *block)
    {
        _aligned_free(block);
    }

    FixedBlockAllocator::~FixedBlockAllocator()
    {
        assert(this->mMemoryPool == NULL && "Ensure that Teardown() is called before destruction!");
        assert(this->mIndices == NULL);
    }

    void FixedBlockAllocator::Initialize(size_t numElements, size_t elementAlignment, size_t elementSize)
    {
        const size_t alignedElementSize = (elementSize + (elementAlignment - 1)) & ~(elementAlignment - 1);
        const size_t allocationSize = numElements * elementSize;
        this->mMemoryPool = MemAlloc(bc::POOL_COMPONENT, allocationSize);
        size_t * const indices = static_cast<size_t *>(MemAlloc(bc::POOL_COMPONENT, numElements * sizeof(size_t)));
        this->mIndices = indices;
        this->mCurrentIndex = 0;
        this->mNumElements = numElements;
        this->mElementSize = alignedElementSize;
                 
        for (size_t i = 0, e = numElements; i < e; ++i)
            indices[i] = i;
    }

    void FixedBlockAllocator::Teardown()
    {
        assert(mCurrentIndex == 0 && "Component memory pool still has outstanding allocations!");

        bc::MemFree(this->mIndices);
        mIndices = NULL;
        bc::MemFree(this->mMemoryPool);
        mMemoryPool = NULL;
    }

    void *FixedBlockAllocator::Allocate()
    {
        bc::AutoFutex autoFutex(this->mFutex);
        const size_t arrayIndex = this->mCurrentIndex;
        const size_t numElements = this->mNumElements;
        if (arrayIndex >= numElements)
            return NULL;

        const size_t *indices = this->mIndices;
        const size_t elementSize = this->mElementSize;
        char *pool = static_cast<char *>(this->mMemoryPool);

        this->mCurrentIndex = arrayIndex + 1;
        const size_t index = indices[arrayIndex];
        return pool + index * elementSize;
    }

    void FixedBlockAllocator::Free(void *object)
    {
        const size_t elementSize = this->mElementSize;
        char * const memoryPool = static_cast<char *>(this->mMemoryPool);

        const uintptr_t objectPoolIndex = static_cast<uintptr_t>(
            static_cast<char *>(object) - static_cast<char *>(memoryPool)) / elementSize;
        assert(objectPoolIndex < this->mNumElements);

        {
            bc::AutoFutex autoFutex(mFutex);

            size_t rosterPosition, e;
            const size_t currentIndex = this->mCurrentIndex;
            size_t * const indices = this->mIndices;

            for (rosterPosition = 0, e = currentIndex; rosterPosition < e; ++rosterPosition)
                if (indices[rosterPosition] == objectPoolIndex)
                {
                    break;
                }

            const size_t lastIndex = currentIndex - 1;
            assert(rosterPosition <= lastIndex);
            const size_t lastIndexValue = indices[lastIndex];

            indices[rosterPosition] = lastIndexValue;
            indices[lastIndex] = objectPoolIndex;
            this->mCurrentIndex = lastIndex;
        }
    }

    void *FixedBlockAllocator::GetObjectAtIndexImpl(size_t index)
    {
        assert(index < this->mCurrentIndex);
        char *base = static_cast<char *>(this->mMemoryPool);
        const size_t elementSize = this->mElementSize;
        return base + (elementSize * index);
    }

    struct InternTableEntry
    {
        uint64_t mHash;
        const char *mString;
    };

    struct InternTable
    {
        InternTable *mNext;
        char *mPtr;
        InternTableEntry *mInternTableEntry;
        InternTableEntry *mInternTableEntryEnd;
        char mBuffer[4096 - 4 * sizeof(void *)];
    };

    static InternTable *AllocateInternTable()
    {
        InternTable *table = static_cast<InternTable *>(VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        table->mNext = 0;
        table->mPtr = table->mBuffer;
        table->mInternTableEntryEnd = (InternTableEntry *)((char *)table + 4096);
        table->mInternTableEntry = table->mInternTableEntryEnd;

        return table;
    }

    static bool EntryComparer(const InternTableEntry &a, const InternTableEntry &b)
    {
        return a.mHash < b.mHash;
    }

    static const char *SearchTables(InternTable *table, uint64_t hash)
    {
        if (table == NULL)
            return NULL;

        InternTableEntry target = { hash, 0 };

        std::pair<InternTableEntry *, InternTableEntry *> bounds = std::equal_range(
                table->mInternTableEntry,
                table->mInternTableEntryEnd,
                target, 
                EntryComparer);

        if (bounds.first < bounds.second)
            return bounds.first->mString;
        return SearchTables(table->mNext, hash);
    }

    const char *Intern(const char **outPtr, uint64_t *outHash, const char *begin, const char *end)
    {
        static_assert(sizeof(InternTable) == 4096, "Intern table entries must be page-sized");
        static InternTable *gInternTable;

        if (gInternTable == NULL)
            gInternTable = AllocateInternTable();

        uint64_t stringHash = Hash(begin, end);
        const char *string = SearchTables(gInternTable, stringHash);
        if (string)
            goto finish;

        // This calculation should be end - begin + sizeof(void *) - 1, but it needs to take into account the 
        // size of the null at the end. Adding the +1 for the null cancels out the - 1.
        const size_t allocationSize = (static_cast<size_t>(end - begin) + sizeof(void *)) & ~(sizeof(void *) - 1);
        const size_t max_length = sizeof(gInternTable->mBuffer) - sizeof(InternTableEntry);
        assert(allocationSize < max_length);

        const size_t required_space = allocationSize + 1 + sizeof(InternTableEntry);
        const size_t available_space = reinterpret_cast<uintptr_t>(gInternTable->mInternTableEntry) 
            - reinterpret_cast<uintptr_t>(gInternTable->mPtr);
        if (required_space > available_space)
        {
            InternTable *table = AllocateInternTable();
            table->mNext = gInternTable;
            gInternTable = table;
        }
        string = gInternTable->mPtr;

        const size_t length = static_cast<size_t>(end - begin);
        memcpy(const_cast<char *>(string), begin, length);
        gInternTable->mPtr[length] = 0;
        gInternTable->mPtr += allocationSize;

        InternTableEntry *entry = --gInternTable->mInternTableEntry;
        entry->mHash = stringHash;
        entry->mString = string;
        std::sort(gInternTable->mInternTableEntry, gInternTable->mInternTableEntryEnd, EntryComparer);

    finish:
        if (outHash)
            *outHash = stringHash;
        if (outPtr)
            *outPtr = string;

        return string;
    }

    const char *Intern(const char **out, uint64_t *outHash, const char *str)
    {
        return Intern(out, outHash, str, str + strlen(str));
    }
    
    static uint64_t gTotalTicks;
    static uint64_t gLastFrameTicks;
    static uint64_t gCurrentFrameTicks;

    void UpdateFrameTime()
    {
        gLastFrameTicks = gCurrentFrameTicks;
        QueryPerformanceCounter((LARGE_INTEGER *)&gCurrentFrameTicks);
    }

    uint64_t GetTotalTicks()
    {
        return gTotalTicks;
    }

    uint64_t GetFrameTicks()
    {
        return gCurrentFrameTicks - gLastFrameTicks;
    }

    uint64_t GetFrequency()
    {
        static uint64_t gFrequency;

        if (gFrequency == 0)
            QueryPerformanceFrequency((LARGE_INTEGER *)&gFrequency);

        return gFrequency;
    }

    void Report(const char *format, ...)
    {
        char errorBuf[1024];

        va_list args;
        va_start(args, format);
        vsnprintf(errorBuf, sizeof errorBuf, format, args);
        va_end(args);

        errorBuf[1023] = 0;

        OutputDebugString(errorBuf);
        OutputDebugString("\n");
        puts(errorBuf);
    }

    void ReportError(const char *file, int line, const char *errorFormat, ...)
    {
        char errorBuf[1024];

        int count = _snprintf(errorBuf, sizeof errorBuf, "%s(%d): RUNTIME ERROR: ", file, line);

        va_list args;
        va_start(args, errorFormat);
        vsnprintf(errorBuf + count, sizeof errorBuf - count, errorFormat, args);
        va_end(args);

        errorBuf[1023] = 0;

        OutputDebugString(errorBuf);
        OutputDebugString("\n");
        puts(errorBuf);

        if (IsDebuggerPresent())
            __debugbreak();
    }
}
