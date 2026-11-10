#pragma once

#include <mutex>
#include <string>

struct ModelSettings
{
    std::string provider = "deepseek";
    std::string model = "deepseek-flash";
};

class ModelConfig
{
public:
    ModelConfig();

    // Load the user's saved API Key, retaining compatibility with the old environment variable.
    bool loadApiKey();
    std::string getApiKey() const;
    bool updateApiKey(const std::string& newApiKey);
    bool saveApiKey(const std::string& newApiKey);
    bool hasApiKey() const;

    // Provider and model selection are persisted independently from the API Key.
    bool loadModelSettings();
    ModelSettings getModelSettings() const;
    std::string getModelProvider() const;
    std::string getModelName() const;
    bool saveModelSettings(
        const std::string& provider,
        const std::string& model
    );
    bool isModelConfigured() const;

    std::string getOllamaBaseUrl() const;

private:
    mutable std::mutex apiKeyMutex;
    std::mutex apiKeyPersistenceMutex;
    std::string apiKey;

    mutable std::mutex modelSettingsMutex;
    std::mutex modelSettingsPersistenceMutex;
    std::string modelProvider;
    std::string modelName;
};
