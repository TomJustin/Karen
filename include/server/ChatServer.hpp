#pragma once

#include "chat/Conversation.hpp"
#include "model/AIModel.hpp"
#include "model/ModelConfig.hpp"
#include "user/Profile.hpp"
#include "storage/ConversationStorage.hpp"
#include "storage/MemoryStorage.hpp"
#include "memory/MemoryExtractor.hpp"

#include <mutex>

class ChatServer
{
public:
    ChatServer(
        Conversation& conversation,
        Profile& profile,
        MemoryStore& memoryStore,
        AIModel& model,
        ModelConfig& config,
        ConversationStorage& conversationStorage,
        MemoryStorage& memoryStorage,
        MemoryExtractor& extractor
    );

    void start();

private:
    Conversation& conversation;
    Profile& profile;
    MemoryStore& memoryStore;
    AIModel& model;
    ModelConfig& config;

    ConversationStorage& conversationStorage;
    MemoryStorage& memoryStorage;
    MemoryExtractor& extractor;

    // Profile settings can be changed while chat requests are being handled.
    std::mutex profileMutex;
};
