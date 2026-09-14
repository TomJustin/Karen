#pragma once

#include <string>

#include "chat/Conversation.hpp"
#include "user/Profile.hpp"
#include "memory/MemoryStore.hpp"

class AIModel{
    public:

    virtual std::string chat(
    const Conversation& conversation,
    const Profile& profile,
    const MemoryStore& memoryStore
) = 0;

virtual std::string chat(
    const std::vector<Message>& messages
) = 0;

};