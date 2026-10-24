#pragma once

#include "memory/MemoryStore.hpp"

class MemoryStorage
{
public:
    void saveMemories(const MemoryStore& memoryStore) const;

    MemoryStore loadMemories() const;
};
