#include <iostream>
#include <memory>

#include <windows.h>

#include "user/Profile.hpp"

#include "storage/ConversationStorage.hpp"
#include "storage/ProfileStorage.hpp"
#include "storage/MemoryStorage.hpp"

#include "model/ModelFactory.hpp"
#include "model/ModelConfig.hpp"

#include "memory/MemoryExtractor.hpp"

#include "server/ChatServer.hpp"


int main()
{
    // =========================
    // Windows UTF-8
    // =========================

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);


    // =========================
    // 创建基础组件
    // =========================

    Profile profile;

    ConversationStorage conversationStorage;

    ProfileStorage profileStorage;

    MemoryStorage memoryStorage;


    // =========================
    // 1. 读取聊天记录
    // =========================

    Conversation conversation =
        conversationStorage.loadConversation();


    // =========================
    // 2. 读取 / 初始化 Profile
    // =========================

    if (!profileStorage.loadProfile(profile))
    {
        if (!profileStorage.setupProfile(profile))
        {
            return 1;
        }
    }


    // =========================
    // 3. 读取 Memory
    // =========================

    MemoryStore memoryStore =
        memoryStorage.loadMemories();


    // =========================
    // 4. 读取 API Key
    // =========================

    ModelConfig config;

    if (!config.loadApiKey())
    {
        std::cout
            << "API NOT FOUND"
            << std::endl;

        return 1;
    }


    // =========================
    // 5. 创建 AI Model
    // =========================

    std::size_t count = 20;

    std::unique_ptr<AIModel> model =
        ModelFactory::createModel(
            config,
            count
        );


    // =========================
    // 6. 创建 MemoryExtractor
    // =========================

    MemoryExtractor extractor(
        *model
    );


    // =========================
    // 7. 创建服务器
    // =========================

    ChatServer server(
        conversation,
        profile,
        memoryStore,
        *model,
        conversationStorage,
        memoryStorage,
        extractor
    );


    // =========================
    // 8. 启动服务器
    // =========================

    server.start();


    return 0;
}