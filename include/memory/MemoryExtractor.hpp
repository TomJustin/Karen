#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "model/AIModel.hpp"

struct MemoryCandidate
{
    bool shouldRemember = false;

    // Empty means a new memory. Otherwise, targetIndices[0] is the entry
    // to update and later indices are duplicate entries to remove.
    std::vector<std::size_t> targetIndices;

    std::string category;
    std::string content;
};

class MemoryExtractor
{
public:
    MemoryExtractor(AIModel& model);

    MemoryCandidate extract(
        const std::string& userMessage,
        const std::vector<Memory>& existingMemories
    );

private:
    AIModel& model;
};
