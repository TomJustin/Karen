#include "storage/ConversationStorage.hpp"

#include <fstream>

void ConversationStorage::saveConversation(const Conversation& conversation){
    std::ofstream file("data/message.json");

    file << conversation.toJson().dump(4);
}