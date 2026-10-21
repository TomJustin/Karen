#include "storage/MemoryStorage.hpp"
#include "storage/AppDataPaths.hpp"

#include <fstream>
#include <stdexcept>

void MemoryStorage::saveMemories(const MemoryStore& memoryStore)
{
    std::ofstream file(
        KarenPaths::getDataFilePath("memory.json"),
        std::ios::binary | std::ios::trunc
    );

    if (!file)
    {
        throw std::runtime_error("无法打开长期记忆文件进行保存");
    }

    file << memoryStore.toJson().dump(4);

    if (!file)
    {
        throw std::runtime_error("写入长期记忆失败");
    }
}

MemoryStore MemoryStorage::loadMemories() const
{
    std::ifstream file(
        KarenPaths::getDataFilePathForRead("memory.json"),
        std::ios::binary
    );

    MemoryStore memoryStore;

    if (!file)
    {
        return memoryStore;
    }

    nlohmann::json data;
    file >> data;

    memoryStore.fromJson(data);
    return memoryStore;
}
