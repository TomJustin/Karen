#include "model/ModelFactory.hpp"
#include "model/DeepSeek.hpp"

std::unique_ptr<AIModel> ModelFactory::createModel(
    const ModelConfig& config,
    std::size_t contextMessageCount
)
{
    if (config.getModelName() == "deepseek")
    {
        return std::make_unique<DeepSeek>(
            config.getApiKey(),
            contextMessageCount
        );
    }

    return nullptr;
} 