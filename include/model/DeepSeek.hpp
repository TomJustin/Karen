#pragma once

#include "model/AIModel.hpp"
#include "network/HttpClient.hpp"
#include "model/ModelConfig.hpp"

#include <iostream>

class DeepSeek : public AIModel
{
    public:

    DeepSeek(const ModelConfig& config, std::size_t contextMessageCount);

    std::string chat(
    const Conversation& conversation,
    const Profile& profile,
    const MemoryStore& memoryStore
) override;

    std::string chat(
            const std::vector<Message>& messages
) override;

    private:

    const ModelConfig& config;

    HttpClient httpClient;

    std::size_t contextMessageCount;
};