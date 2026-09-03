#include "model/DeepSeek.hpp"

    DeepSeek::DeepSeek(const std::string& apiKey) : apiKey(apiKey){}

    std::string DeepSeek::chat(const Conversation& conversation){
        nlohmann::json messages = conversation.toJson();

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

        std::cout << "Total tokens : " << responseJson["usage"]["total_tokens"].get<int>() << std::endl;

        return responseJson["choices"][0]["message"]["content"].get<std::string>();
    }