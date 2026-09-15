#include <iostream>

#include <windows.h>
#include <memory>

#include "user/Profile.hpp"
#include "storage/ConversationStorage.hpp"
#include "storage/ProfileStorage.hpp"
#include "storage/MemoryStorage.hpp"
#include "model/ModelFactory.hpp"
#include "model/ModelConfig.hpp"
#include "memory/MemoryExtractor.hpp"



int main()
{
    // 解决 Windows 控制台中文编码问题
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

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
    // 2. 读取 / 初始化Profile
    // =========================

    if(!profileStorage.loadProfile(profile)){
        if(!profileStorage.setupProfile(profile)){
            return 1;
        }
    }
    
    // =========================
    // 3. 读取 / 初始化Memory
    // =========================

    MemoryStore memoryStore = memoryStorage.loadMemories();


    // =========================
    // 4. 获取 API Key
    // =========================

    ModelConfig config;

    if (!config.loadApiKey())
    {
        std::cout << "API NOT FOUND"
                << std::endl;

        return 1;
    }

    const std::string apiKey =
        config.getApiKey();

        std::size_t count;

        std::cout << "请输入想要的记忆长度(轮数 * 2)：";
        std::cin >> count; std::cin.ignore();//忽略回车，避免传入到input

        std::unique_ptr<AIModel> model =
        ModelFactory::createModel(
            config,
            count
        );

        MemoryExtractor extractor(*model);

    // =========================
    // 5. 开始聊天
    // =========================

    while (true)
    {
        std::string input;

        std::getline(std::cin, input);

        // 退出
        if (input == "exit" || input == "退出")
        {
            break;
        }

        // 用户消息
        Message message;

        message.role = "user";
        message.content = input;

        conversation.addMessage(message);

        // 请求 AI
        std::string answer =
            model->chat(
                conversation,
                profile,
                memoryStore
            );

        // AI 消息
        Message assistantMessage;

        assistantMessage.role = "assistant";
        assistantMessage.content = answer;

        conversation.addMessage(
            assistantMessage
        );

        // 保存聊天记录
        conversationStorage.saveConversation(conversation);

        //判断是否值得记忆
        MemoryCandidate candidate = extractor.extract(input);
        if(candidate.shouldRemember){
            Memory memory;
            
            memory.category = candidate.category;
            memory.content = candidate.content;

            memoryStore.addMemory(memory);

            memoryStorage.saveMemories(memoryStore);
        }


        // 输出 AI 回复
        std::cout << profile.getAiName() << ": " << answer << std::endl;
    }

    return 0;
}

