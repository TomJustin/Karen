#include "storage/ConversationStorage.hpp"
#include "storage/AppDataPaths.hpp"

#include <fstream>
#include <stdexcept>

void ConversationStorage::saveConversation(
    const Conversation& conversation
)
{
    std::ofstream file(
        KarenPaths::getDataFilePath("message.json"),
        std::ios::binary | std::ios::trunc
    );

    if (!file)
    {
        throw std::runtime_error("无法打开聊天记录文件进行保存");
    }

    file << conversation.toJson().dump(4);

    if (!file)
    {
        throw std::runtime_error("写入聊天记录失败");
    }
}

Conversation ConversationStorage::loadConversation() const
{
    Conversation conversation;

    std::ifstream file(
        KarenPaths::getDataFilePathForRead("message.json"),
        std::ios::binary
    );

    if (!file)
    {
        return conversation;
    }

    nlohmann::json data;
    file >> data;

    conversation.fromJson(data);
    return conversation;
}
