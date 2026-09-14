#pragma once

#include <string>

class ModelConfig
{
public:
    ModelConfig();

    bool loadApiKey();

    std::string getApiKey() const;
    std::string getModelName() const;

private:
    std::string apiKey;
    std::string modelName;
};