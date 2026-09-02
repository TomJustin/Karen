#include "chat/Conversation.hpp"

    void Conversation::addMessage(const Message& message){
        messages.push_back(message);
    }

    const std::vector<Message>& Conversation::getMessages() const{
        return messages;
    }

    nlohmann::json Conversation::toJson(){
        nlohmann::json Fdata;
        for(const auto& message : messages){
            Fdata.push_back(message.toJson());
        }
        return Fdata;
    }