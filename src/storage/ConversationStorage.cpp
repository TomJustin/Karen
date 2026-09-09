#include "storage/ConversationStorage.hpp"

#include <fstream>

void ConversationStorage::saveConversation(const Conversation& conversation){
    std::ofstream file("data/message.json");

    file << conversation.toJson().dump(4);
    //dump是转换为字符串
}

Conversation ConversationStorage::loadConversation() const{
    Conversation conversation;

    std::ifstream file("data/message.json");

    if(!file){
        return conversation;
    }

    nlohmann::json data;

    file >> data;

    conversation.fromJson(data);

    return conversation;
}