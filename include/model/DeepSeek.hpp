#pragma once

#include "chat/Conversation.hpp"
#include "network/HttpClient.hpp"
#include "user/Profile.hpp"
#include <iostream>

class DeepSeek{
    public:

    DeepSeek(const std::string& apiKey, std::size_t contextMessageCount);

    std::string chat(const Conversation& conversation, const Profile& profile);

    private:

    std::string apiKey;

    HttpClient httpClient;

    std::size_t contextMessageCount;
};