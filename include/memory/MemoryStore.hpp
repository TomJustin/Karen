#pragma once

#include "Memory.hpp"

#include <cstddef>
#include <vector>

class MemoryStore
{
private:
    std::vector<Memory> store;

public:
    // Returns false when the memory is empty or an equivalent entry exists.
    bool addMemory(const Memory& memory);

    // Replaces an existing memory and optionally removes other entries that
    // the extractor identified as duplicates of the same category.
    bool updateMemory(
        std::size_t index,
        const Memory& memory,
        const std::vector<std::size_t>& duplicateIndices = {}
    );

    const std::vector<Memory>& getMemories() const;

    nlohmann::json toJson() const;

    void fromJson(const nlohmann::json& data);

    void clear();

    std::size_t size() const;

    std::vector<Memory> getMemoriesByCategory(const std::string& category) const;

    bool removeMemory(std::size_t index);
};
