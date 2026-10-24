#include "memory/MemoryStore.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace
{
    std::string normalizeMemoryContent(const std::string& value)
    {
        std::string normalized;
        normalized.reserve(value.size());

        bool previousWasWhitespace = true;

        for (unsigned char ch : value)
        {
            if (std::isspace(ch) != 0)
            {
                if (!normalized.empty() && !previousWasWhitespace)
                {
                    normalized.push_back(' ');
                    previousWasWhitespace = true;
                }

                continue;
            }

            if (ch < 0x80)
            {
                normalized.push_back(
                    static_cast<char>(std::tolower(ch))
                );
            }
            else
            {
                normalized.push_back(static_cast<char>(ch));
            }

            previousWasWhitespace = false;
        }

        if (!normalized.empty() && normalized.back() == ' ')
        {
            normalized.pop_back();
        }

        // Ignore only trailing sentence punctuation. Keep internal punctuation
        // intact so terms such as "C++" are not changed.
        const std::vector<std::string> trailingPunctuation = {
            "。", "！", "？", "；", "：", "，", "、",
            ".", "!", "?", ";", ":", ","
        };

        bool removedPunctuation = true;
        while (removedPunctuation && !normalized.empty())
        {
            removedPunctuation = false;

            for (const auto& punctuation : trailingPunctuation)
            {
                if (normalized.size() >= punctuation.size() &&
                    normalized.compare(
                        normalized.size() - punctuation.size(),
                        punctuation.size(),
                        punctuation
                    ) == 0)
                {
                    normalized.erase(
                        normalized.size() - punctuation.size()
                    );
                    removedPunctuation = true;
                    break;
                }
            }
        }

        return normalized;
    }
}

bool MemoryStore::addMemory(const Memory& memory)
{
    if (normalizeMemoryContent(memory.content).empty())
    {
        return false;
    }

    const std::string newKey = normalizeMemoryContent(memory.content);

    for (const auto& existing : store)
    {
        // Same text under a different category can represent a distinct fact.
        if (existing.category == memory.category &&
            normalizeMemoryContent(existing.content) == newKey)
        {
            return false;
        }
    }

    store.push_back(memory);
    return true;
}

bool MemoryStore::updateMemory(
    std::size_t index,
    const Memory& memory,
    const std::vector<std::size_t>& duplicateIndices
)
{
    if (index >= store.size() ||
        normalizeMemoryContent(memory.content).empty())
    {
        return false;
    }

    bool changed =
        store[index].category != memory.category ||
        store[index].content != memory.content;

    // Only remove entries explicitly identified by the extractor, and only
    // when they are in the same category as the memory being updated.
    std::vector<std::size_t> indicesToRemove;
    for (const std::size_t duplicateIndex : duplicateIndices)
    {
        if (duplicateIndex < store.size() &&
            duplicateIndex != index &&
            store[duplicateIndex].category == memory.category)
        {
            indicesToRemove.push_back(duplicateIndex);
        }
    }

    std::sort(indicesToRemove.begin(), indicesToRemove.end());
    indicesToRemove.erase(
        std::unique(indicesToRemove.begin(), indicesToRemove.end()),
        indicesToRemove.end()
    );

    store[index] = memory;
    std::size_t keptIndex = index;

    // Erase from highest to lowest so each removal leaves lower indices valid.
    for (auto it = indicesToRemove.rbegin();
         it != indicesToRemove.rend();
         ++it)
    {
        if (*it < keptIndex)
        {
            --keptIndex;
        }

        store.erase(store.begin() + *it);
        changed = true;
    }

    // A model update can also become text-identical to another record.
    // Collapse those exact duplicates deterministically within the same category.
    const std::string newKey = normalizeMemoryContent(memory.content);
    for (std::size_t current = store.size(); current-- > 0;)
    {
        if (current == keptIndex ||
            store[current].category != memory.category ||
            normalizeMemoryContent(store[current].content) != newKey)
        {
            continue;
        }

        if (current < keptIndex)
        {
            --keptIndex;
        }

        store.erase(store.begin() + current);
        changed = true;
    }

    return changed;
}

const std::vector<Memory>& MemoryStore::getMemories() const
{
    return store;
}

nlohmann::json MemoryStore::toJson() const
{
    nlohmann::json data = nlohmann::json::array();

    for (const auto& cell : store)
    {
        data.push_back(cell.toJson());
    }

    return data;
}

void MemoryStore::fromJson(const nlohmann::json& data)
{
    store.clear();

    if (!data.is_array())
    {
        return;
    }

    // Route loaded data through addMemory too, so exact duplicates from older
    // versions are removed the next time the memory file is saved.
    for (const auto& cell : data)
    {
        if (!cell.is_object() ||
            !cell.contains("category") ||
            !cell["category"].is_string() ||
            !cell.contains("content") ||
            !cell["content"].is_string())
        {
            continue;
        }

        addMemory(Memory::fromJson(cell));
    }
}

void MemoryStore::clear()
{
    store.clear();
}

std::size_t MemoryStore::size() const
{
    return store.size();
}

std::vector<Memory> MemoryStore::getMemoriesByCategory(
    const std::string& category
) const
{
    std::vector<Memory> memory;

    for (const auto& cell : store)
    {
        if (cell.category == category)
        {
            memory.push_back(cell);
        }
    }

    return memory;
}

bool MemoryStore::removeMemory(std::size_t index)
{
    if (index >= store.size())
    {
        return false;
    }

    store.erase(store.begin() + index);
    return true;
}
