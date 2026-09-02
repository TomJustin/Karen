#include <iostream>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include "Message.hpp"
#include "chat/Conversation.hpp"
#include "network/HttpClient.hpp"


int main()
{
    const char* key = std::getenv("DEEPSEEK_API_KEY");
    if(key == nullptr){
        std::cout << "NOT FOUND" << std::endl;
        return 1;
    }
    else{
        std::cout << "API LOADED" << std::endl;
    }
    const std::string apiKey = key;
    std::string url = "https://example.com";
    HttpClient client;

    std::string response;

    Message message;
    message.content = "Hello";
    message.role = "user";

    Conversation conversation;
    conversation.addMessage(message);

    nlohmann::json request;
    request["model"] = "deepseek";
    request["messages"] = conversation.toJson();

    std::string body = request.dump();
    response = client.post(url, body, apiKey);
    std::cout << response << std::endl;
    return 0;
}
