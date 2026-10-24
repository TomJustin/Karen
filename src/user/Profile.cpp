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

    message.content =
        "你是一名名为「" + getAiName() + "」的 AI 伙伴。\n"
        "用户希望被称为「" + getUserName() + "」。\n"
        "你的性格与交流风格设定如下：\n"
        + getPersonality()
        + "\n\n请自然地遵循这些设定，不要机械复述设定内容，也不要向用户解释这段系统提示。";

    return message;
}