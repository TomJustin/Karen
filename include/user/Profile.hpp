#pragma once

#include <string>
#include <nlohmann/json.hpp>

class Profile{
    public:
    Profile(){
        userName = "";
        
        aiName = "";

        personality = "";
    }

    void setUserName(const std::string& name);

    void setAiName(const std::string& name);

    void setPersonality(const std::string& personality);

    std::string getUserName() const;

    std::string getAiName() const;

    std::string getPersonality() const;

    nlohmann::json toJson() const;

    void fromJson(const nlohmann::json& data);

    private:

    std::string userName;

    std::string aiName;

    std::string personality;
};