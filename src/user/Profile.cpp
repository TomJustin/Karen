#include "user/Profile.hpp"

nlohmann::json Profile::toJson() const{
    nlohmann::json data;

    data["userName"] = userName;

    data["aiName"] = aiName;
    
    data["personality"] = personality;

    return data;
}

void Profile::fromJson(const nlohmann::json& data){
    userName = data["userName"].get<std::string>();

    aiName = data["aiName"].get<std::string>();

    personality = data["personality"].get<std::string>();

}

void Profile::setUserName(const std::string& name){
    this->userName = name;
}

void Profile::setAiName(const std::string& name){
    this->aiName = name;
}

void Profile::setPersonality(const std::string& personality){
    this->personality = personality;
}

std::string Profile::getUserName() const{
    return userName;
}

std::string Profile::getAiName() const{
    return aiName;
}

std::string Profile::getPersonality() const{
    return personality;
}

Message Profile::getSystemMessage() const{
    Message message;

    message.role = "system";

    message.content = "用户的名字叫做：" + this->getUserName() + "ai的名字叫做：" + this->getAiName() + "ai的性格是：" + this->getPersonality();
    
    return message;
}