#pragma once

#include <mutex>
#include <string>

class ModelConfig
{
public:
    ModelConfig();

    // 读取已保存的 Key；没有时兼容旧的环境变量配置
    bool loadApiKey();

    std::string getApiKey() const;
    std::string getModelName() const;

    // 只更新当前进程的内存配置
    bool updateApiKey(const std::string& newApiKey);

    // 将 Key 使用 Windows DPAPI 加密保存，并更新内存配置
    bool saveApiKey(const std::string& newApiKey);

    bool hasApiKey() const;

private:
    mutable std::mutex apiKeyMutex;
    std::mutex apiKeyPersistenceMutex;
    std::string apiKey;
    std::string modelName;
};
