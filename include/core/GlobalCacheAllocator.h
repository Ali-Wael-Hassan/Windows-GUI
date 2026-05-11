#pragma once

#include <cstdint>
#include <cstdlib>
#include <assert.h>

class GlobalCacheAllocator {
private:
    uint8_t* buffer = nullptr;
    size_t capacity = 0;
    size_t offset   = 0;

    GlobalCacheAllocator(size_t size) : capacity(size), offset(0) {
#if defined(_WIN32)
        buffer = reinterpret_cast<uint8_t*>(_aligned_malloc(size, 64));
#else
        buffer = reinterpret_cast<uint8_t*>(std::aligned_alloc(64, (size + 63) & ~63));
#endif
        assert(buffer != nullptr && "Can't allocate the bufer");
    }

public:
    // Singleton access
    static GlobalCacheAllocator& getInstance(size_t size = 1024 * 1024) {
        static GlobalCacheAllocator instance(size);
        return instance;
    }

    // Non-copyable
    GlobalCacheAllocator(const GlobalCacheAllocator&) = delete;
    GlobalCacheAllocator& operator=(const GlobalCacheAllocator&) = delete;

    ~GlobalCacheAllocator() {
#if defined(_WIN32)
        _aligned_free(buffer);
#else
        free(buffer);
#endif
    }

    // Allocate 'bytes' from the buffer
    uint8_t* allocate(size_t bytes, size_t alignment = 64) {
        size_t alignedOffset = (offset + alignment - 1) & ~(alignment - 1);
        if (alignedOffset + bytes > capacity) return nullptr;
        uint8_t* ptr = buffer + alignedOffset;
        offset = alignedOffset + bytes;
        return ptr;
    }

    // Reset all allocations
    void reset() { offset = 0; }

    // Access the raw buffer
    uint8_t* getBuffer() const { return buffer; }
    size_t getCapacity() const { return capacity; }
    size_t getOffset() const { return offset; }
};