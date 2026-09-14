#include "model/DeepSeek.hpp"
#include <iostream>

    DeepSeek::DeepSeek(const std::string& apiKey,
         std::size_t contextMessageCount) : apiKey(apiKey),
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

        request["model"] = "deepseek-v4-flash";

        for (const auto& message : messages){
        request["messages"].push_back(
            message.toJson()
        );
    }

        std::string body = request.dump();

        std::string response = httpClient.post(
            "https://api.deepseek.com/chat/completions",
            body,
            apiKey
        );

        nlohmann::json responseJson = nlohmann::json::parse(response);

        if(responseJson.contains("error")){
            std::cout << responseJson["error"]["message"] << std::endl;

            return "";
        }

        if(!responseJson.contains("choices")){
            std::cout << "There is no choices!" << std::endl;
            return "";
        }

        if (responseJson["choices"].empty()){
            std::cout << "Choices is empty." << std::endl;
            return "";
        }

        return responseJson["choices"][0]["message"]["content"].get<std::string>();
}