#include "Message.hpp"

nlohmann::json Message::toJson() const {
    nlohmann::json data;
    data["role"] = role;
    data["content"] = content;
    return data;
}