#include "model/ModelConfig.hpp"
#include <cstdlib>

ModelConfig::ModelConfig()
{
    modelName = "deepseek";
}

bool ModelConfig::loadApiKey(){
    const char* key =
        std::getenv("DEEPSEEK_API_KEY");

    if (key == nullptr)
    {
        return false;
    }

    apiKey = key;

    return true;
}

std::string ModelConfig::getApiKey() const
{
    return apiKey;
}

std::string ModelConfig::getModelName() const
{
    return modelName;
}