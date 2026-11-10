#include "model/ModelFactory.hpp"
#include "model/DeepSeek.hpp"
#include "model/Ollama.hpp"

#include <stdexcept>

std::unique_ptr<AIModel> ModelFactory::createModel(
    const ModelConfig& config,
    std::size_t contextMessageCount
)
{
    const std::string provider = config.getModelProvider();

    if (provider == "deepseek")
    {
        return std::make_unique<DeepSeek>(
            config,
            contextMessageCount
        );
    }

    if (provider == "ollama")
    {
        return std::make_unique<Ollama>(
            config,
            contextMessageCount
        );
    }

    throw std::runtime_error("不支持的 AI 模型提供商：" + provider);
}
