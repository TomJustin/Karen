#pragma once

#include "model/AIModel.hpp"
#include "model/ModelConfig.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>

class ModelManager : public AIModel
{
public:
    ModelManager(
        const ModelConfig& config,
        std::size_t contextMessageCount
    );

    std::string chat(
        const Conversation& conversation,
        const Profile& profile,
        const MemoryStore& memoryStore
    ) override;

    std::string chat(
        const std::vector<Message>& messages
    ) override;

private:
    std::shared_ptr<AIModel> getActiveModel();

    const ModelConfig& config;
    const std::size_t contextMessageCount;

    std::mutex modelMutex;
    std::string activeSettingsKey;
    std::shared_ptr<AIModel> activeModel;
};
