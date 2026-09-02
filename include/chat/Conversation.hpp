#pragma once

#include <vector>
#include <nlohmann/json.hpp>
#include "Message.hpp"

class Conversation
{
private:

    std::vector<Message> messages;

public:

    void addMessage(const Message& message);

    const std::vector<Message>& getMessages() const;

    nlohmann::json toJson();
};