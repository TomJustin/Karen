#include "chat/Message.hpp"

nlohmann::json Message::toJson() const {
    nlohmann::json data;
    data["role"] = role;
    data["content"] = content;
    return data;
}

Message Message::fromJson(const nlohmann::json& data){
    Message message;

    message.content = data["content"];

    message.role = data["role"];

    return message;
}