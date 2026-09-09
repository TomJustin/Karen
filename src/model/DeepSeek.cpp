#include "model/DeepSeek.hpp"

    DeepSeek::DeepSeek(const std::string& apiKey,
         std::size_t contextMessageCount) : apiKey(apiKey),
          contextMessageCount(contextMessageCount){}

    std::string DeepSeek::chat(const Conversation& conversation, const Profile& profile){

        Message systemMessage = profile.getSystemMessage();

        nlohmann::json messages = nlohmann::json::array();

        messages.push_back(systemMessage.toJson());

        std::vector<Message> recentMessages =
    conversation.getRecentMessages(contextMessageCount);

        for(const auto& cell : recentMessages){
            messages.push_back(cell.toJson());
        }

        nlohmann::json request;

        request["model"] = "deepseek-v4-flash";
        request["messages"] = messages;

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