#include "model/ModelManager.hpp"
#include "model/ModelFactory.hpp"

#include <stdexcept>
#include <utility>

ModelManager::ModelManager(
    const ModelConfig& config,
    std::size_t contextMessageCount
)
    : config(config),
      contextMessageCount(contextMessageCount)
{
}

std::shared_ptr<AIModel> ModelManager::getActiveModel()
{
    const ModelSettings settings = config.getModelSettings();
    const std::string settingsKey =
        settings.provider + "\n" + settings.model;

    std::lock_guard<std::mutex> lock(modelMutex);

    if (!activeModel || activeSettingsKey != settingsKey)
    {
        std::unique_ptr<AIModel> created =
            ModelFactory::createModel(config, contextMessageCount);

        if (!created)
        {
            throw std::runtime_error("无法创建所选 AI 模型。");
        }

        activeModel = std::shared_ptr<AIModel>(std::move(created));
        activeSettingsKey = settingsKey;
    }

    return activeModel;
}

std::string ModelManager::chat(
    const Conversation& conversation,
    const Profile& profile,
    const MemoryStore& memoryStore
)
{
    return getActiveModel()->chat(conversation, profile, memoryStore);
}

std::string ModelManager::chat(const std::vector<Message>& messages)
{
    return getActiveModel()->chat(messages);
}
