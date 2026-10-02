#pragma once

#include "chat/Conversation.hpp"
#include "model/AIModel.hpp"
#include "user/Profile.hpp"
#include "storage/ConversationStorage.hpp"
#include "storage/MemoryStorage.hpp"
#include "memory/MemoryExtractor.hpp"

class ChatServer
{
public:

    ChatServer(
        Conversation& conversation,
        Profile& profile,
        MemoryStore& memoryStore,
        AIModel& model,
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

    ConversationStorage& conversationStorage;
    MemoryStorage& memoryStorage;

    MemoryExtractor& extractor;
};