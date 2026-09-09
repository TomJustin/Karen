#include "memory/Memory.hpp"

nlohmann::json Memory::toJson() const
{
    nlohmann::json data;

    data["category"] = category;

    data["content"] = content;

    return data;
}

Memory Memory::fromJson(
    const nlohmann::json& data
)
{
    Memory memory;

    memory.category = data["category"].get<std::string>();

    memory.content = data["content"].get<std::string>();

    return memory;
}