#pragma once

#include "Memory.hpp"

#include <vector>

class MemoryStore{
    private:
    std::vector<Memory> store;

    public:
    void addMemory(const Memory& memory);

    const std::vector<Memory>& getMemories() const;

    nlohmann::json toJson() const;

    void fromJson(const nlohmann::json& data);

    void clear();

    std::size_t size() const;
};