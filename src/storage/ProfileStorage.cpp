#include "storage/ProfileStorage.hpp"

bool ProfileStorage::loadProfile(Profile& profile){
    std::ifstream file("data/profile.json");

    if (file)
    {
        std::string content(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        // 文件存在，但是空的
        if (content.empty())
        {
            return false;
        }
        else
        {
            nlohmann::json prodata =
                nlohmann::json::parse(content);

            profile.fromJson(prodata);

            return true;
        }
    }
    else
    {
        // 文件不存在
        return false;
    }

}
    
bool ProfileStorage::setupProfile(Profile& profile){

            std::string userName;
            std::string aiName;
            std::string personality;

            std::cout << "请输入你的名字：";
            std::getline(std::cin, userName);

            std::cout << "请输入 AI 的名字：";
            std::getline(std::cin, aiName);

            std::cout << "请输入 AI 的性格：";
            std::getline(std::cin, personality);

            profile.setUserName(userName);
            profile.setAiName(aiName);
            profile.setPersonality(personality);

            // 保存 Profile
            std::ofstream saveProfile("data/profile.json");

            if (!saveProfile)
            {
                std::cout << "无法保存 profile.json"
                        << std::endl;

                return false;
            }

            saveProfile << profile.toJson().dump(4);

            std::cout << "Profile 设置完成！"
                    << std::endl;

            return true;
}