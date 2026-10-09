#include "chat/Conversation.hpp"
#include <stdexcept>

    void Conversation::addMessage(const Message& message){
        messages.push_back(message);
    }

    const std::vector<Message>& Conversation::getMessages() const{
        return messages;
    }

    nlohmann::json Conversation::toJson() const{
        nlohmann::json Fdata;
        for(const auto& message : messages){
            Fdata.push_back(message.toJson());
        }
        return Fdata;
    }

    void Conversation::fromJson(const nlohmann::json& data){
        messages.clear();

        for(const auto& cell : data){
            messages.push_back(Message::fromJson(cell));
        }
    }

    std::vector<Message> Conversation::getRecentMessages(std::size_t count) const{
        if(messages.size() <= count){
            return messages;
        }

        std::vector<Message> newMessages;

        for(auto i = messages.size() - count ;
         i < messages.size() ;
          i++){
            newMessages.push_back(messages[i]);
        }

        return newMessages;
    }

    void Conversation::removeLastMessage()
    {
        if (!messages.empty())
        {
            messages.pop_back();
        }
    }

    


    void Conversation::removeMessage(size_t index)
    {
        if (index >= messages.size())
        {
            throw std::out_of_range("Message index out of range");
        }

        messages.erase(messages.begin() + index);
    }

    int Conversation::size()
    {
        return messages.size();
    }