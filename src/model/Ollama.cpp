#include "model/Ollama.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <utility>
#include <string>
#include <vector>

using json = nlohmann::json;

Ollama::Ollama(
    const ModelConfig& config,
    std::size_t contextMessageCount
)
    : config(config),
      contextMessageCount(contextMessageCount)
{
}

std::string Ollama::chat(
    const Conversation& conversation,
    const Profile& profile,
    const MemoryStore& memoryStore
)
{
    std::vector<Message> messages;
    messages.push_back(profile.getSystemMessage());

    if (!memoryStore.getMemories().empty())
    {
        Message memoryMessage;
        memoryMessage.role = "system";
        memoryMessage.content = "以下是关于用户的长期记忆：\n";

        for (const auto& memory : memoryStore.getMemories())
        {
            memoryMessage.content +=
                "[" + memory.category + "]: " + memory.content + "\n";
        }

        messages.push_back(std::move(memoryMessage));
    }

    const std::vector<Message> recentMessages =
        conversation.getRecentMessages(contextMessageCount);

    for (const auto& message : recentMessages)
    {
        messages.push_back(message);
    }

    return chat(messages);
}

std::string Ollama::chat(const std::vector<Message>& messages)
{
    json request;
    request["model"] = config.getModelName();
    request["messages"] = json::array();

    for (const auto& message : messages)
    {
        request["messages"].push_back(message.toJson());
    }

    // A single JSON response keeps the existing synchronous chat contract.
    request["stream"] = false;

    const std::string url =
        config.getOllamaBaseUrl() + "/v1/chat/completions";

    // Ollama ignores this placeholder Authorization value. It is not a secret.
    const std::string response = httpClient.post(
        url,
        request.dump(),
        "ollama",
        300000L
    );

    if (response.empty())
    {
        throw std::runtime_error(
            "无法连接本地 Ollama。请确认 Ollama 正在运行，并监听 127.0.0.1:11434。"
        );
    }

    json responseJson;
    try
    {
        responseJson = json::parse(response);
    }
    catch (const json::parse_error&)
    {
        throw std::runtime_error(
            "Ollama 返回了无法解析的响应，请检查本地服务状态。"
        );
    }

    if (responseJson.contains("error"))
    {
        std::string errorMessage = "Ollama 请求失败";

        if (responseJson["error"].is_string())
        {
            errorMessage = responseJson["error"].get<std::string>();
        }
        else if (responseJson["error"].is_object() &&
                 responseJson["error"].contains("message") &&
                 responseJson["error"]["message"].is_string())
        {
            errorMessage =
                responseJson["error"]["message"].get<std::string>();
        }

        throw std::runtime_error("Ollama: " + errorMessage);
    }

    if (!responseJson.contains("choices") ||
        !responseJson["choices"].is_array() ||
        responseJson["choices"].empty())
    {
        throw std::runtime_error(
            "Ollama 返回格式异常：缺少有效的 choices"
        );
    }

    const auto& firstChoice = responseJson["choices"][0];

    if (!firstChoice.contains("message") ||
        !firstChoice["message"].is_object() ||
        !firstChoice["message"].contains("content") ||
        !firstChoice["message"]["content"].is_string())
    {
        throw std::runtime_error(
            "Ollama 返回格式异常：缺少回复内容"
        );
    }

    return firstChoice["message"]["content"].get<std::string>();
}
