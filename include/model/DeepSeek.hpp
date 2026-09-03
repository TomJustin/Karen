#pragma once

#include "chat/Conversation.hpp"
#include "network/HttpClient.hpp"
#include <iostream>

class DeepSeek{
    public:

    DeepSeek(const std::string& apiKey);

    std::string chat(const Conversation& conversation);

    private:

    std::string apiKey;

    HttpClient httpClient;
};