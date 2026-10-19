#include "model/DeepSeek.hpp"
#include <iostream>
#include <stdexcept>

    DeepSeek::DeepSeek(const ModelConfig& config,
         std::size_t contextMessageCount) : config(config),
          contextMessageCount(contextMessageCount){

          }



    std::string DeepSeek::chat(
    const Conversation& conversation,
    const Profile& profile,
    const MemoryStore& memoryStore
){
    std::vector<Message> messages;

    Message systemMessage =
        profile.getSystemMessage();

    messages.push_back(systemMessage);

    if (!memoryStore.getMemories().empty())
    {
        Message memoryMessage;
        memoryMessage.role = "system";

        std::string memoryContent =
            "以下是关于用户的长期记忆：\n";

        for (const auto& cell :
             memoryStore.getMemories())
        {
            memoryContent +=
                "[" + cell.category + "]" +
                ": " + cell.content + "\n";
        }

        memoryMessage.content = memoryContent;

        messages.push_back(memoryMessage);
    }

    std::vector<Message> recentMessages =
        conversation.getRecentMessages(
            contextMessageCount
        );

    for (const auto& cell : recentMessages)
    {
        messages.push_back(cell);
    }

    return chat(messages);
}

    std::string DeepSeek::chat(
            const std::vector<Message>& messages
){
    nlohmann::json request;

        request["model"] = "deepseek-flash";

        for (const auto& message : messages){
        request["messages"].push_back(
            message.toJson()
        );
    }

        std::string body = request.dump();

        std::string response = httpClient.post(
            "https://api.deepseek.com/chat/completions",
            body,
            config.getApiKey()
        );

        if (response.empty())
        {
            throw std::runtime_error(
                "没有收到 DeepSeek API 响应，请检查网络或代理设置"
            );
        }

        nlohmann::json responseJson;

        try
        {
            responseJson = nlohmann::json::parse(response);
        }
        catch (const nlohmann::json::parse_error&)
        {
            throw std::runtime_error(
                "DeepSeek API 返回了无法解析的响应"
            );
        }

        if (responseJson.contains("error"))
        {
            std::string errorMessage = "DeepSeek API 请求失败";

            if (responseJson["error"].is_object() &&
                responseJson["error"].contains("message") &&
                responseJson["error"]["message"].is_string())
            {
                errorMessage =
                    responseJson["error"]["message"].get<std::string>();
            }

            throw std::runtime_error(errorMessage);
        }

        if (!responseJson.contains("choices") ||
            !responseJson["choices"].is_array() ||
            responseJson["choices"].empty())
        {
            throw std::runtime_error(
                "DeepSeek API 返回格式异常：缺少有效的 choices"
            );
        }

        const auto& firstChoice = responseJson["choices"][0];

        if (!firstChoice.contains("message") ||
            !firstChoice["message"].is_object() ||
            !firstChoice["message"].contains("content") ||
            !firstChoice["message"]["content"].is_string())
        {
            throw std::runtime_error(
                "DeepSeek API 返回格式异常：缺少回复内容"
            );
        }

        return firstChoice["message"]["content"].get<std::string>();
}