#pragma once

#include "chat/Conversation.hpp"

class ConversationStorage{
    public:
    void saveConversation(const Conversation& conversation);

    Conversation loadConversation();
};