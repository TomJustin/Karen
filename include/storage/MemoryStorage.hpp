#pragma once

#include "memory/MemoryStore.hpp"

class MemoryStorage{
    public:
    void saveMemories(const MemoryStore& memoryStore);

    MemoryStore loadMemories() const;
};