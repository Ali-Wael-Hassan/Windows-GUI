#pragma once
#include <cstdint>
#include <cstring>
#include "core/GlobalCacheAllocator.h"

template <typename Key, typename Value, uint8_t MAX_SIZE, uint8_t TTL>
class LinearCache {
private:
    uint8_t* m_buffer = nullptr;
    uint8_t  m_count  = 0;
    void (*m_destroy)(Value) = nullptr;

    static constexpr size_t OFFSET_VALUES  = MAX_SIZE * sizeof(Key);
    static constexpr size_t OFFSET_FRAMES  = MAX_SIZE * (sizeof(Key) + sizeof(Value));

    Key* keys()     { return reinterpret_cast<Key*>(m_buffer); }
    Value* values() { return reinterpret_cast<Value*>(m_buffer + OFFSET_VALUES); }
    uint8_t* frames(){ return m_buffer + OFFSET_FRAMES; }

public:
    LinearCache(size_t allocatorBytes, void (*destroy)(Value) = nullptr) 
        : m_destroy(destroy) 
    {
        auto& allocator = GlobalCacheAllocator::getInstance();
        m_buffer = allocator.allocate(allocatorBytes, 64);
        assert(m_buffer != nullptr && "Can't allocate the bufer");
        std::memset(m_buffer, 0, allocatorBytes);
    }

    template<typename CreateFn>
    Value getOrCreate(const Key& key, uint8_t currentFrame, CreateFn create) {
        Key* k = keys();
        Value* v = values();
        uint8_t* f = frames();

        uint8_t evictIdx = 0;
        uint8_t maxAge = 0;

        for (uint8_t i = 0; i < m_count; ++i) {
            if (k[i] == key) {
                f[i] = currentFrame;
                return v[i];
            }

            uint8_t age = currentFrame - f[i];
            if (age >= TTL) {
                evictIdx = i;
                maxAge = age;
                break;
            }
            if (age > maxAge) {
                maxAge = age;
                evictIdx = i;
            }
        }

        if (m_count < MAX_SIZE) {
            k[m_count] = key;
            v[m_count] = create(key);
            f[m_count] = currentFrame;
            return v[m_count++];
        }

        if (m_destroy) m_destroy(v[evictIdx]);

        k[evictIdx] = key;
        v[evictIdx] = create(key);
        f[evictIdx] = currentFrame;

        return v[evictIdx];
    }

    void clear() {
        if (m_destroy) {
            Value* v = values();
            for (uint8_t i = 0; i < m_count; ++i) {
                m_destroy(v[i]);
            }
        }
        m_count = 0;
        std::memset(m_buffer, 0, OFFSET_FRAMES + MAX_SIZE);
    }

    uint8_t size() const { return m_count; }
};