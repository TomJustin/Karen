#include "storage/ProfileStorage.hpp"

#include <filesystem>

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
    
bool ProfileStorage::setupProfile(Profile& profile)
{
    // First launch must not block waiting for console input: Karen is a
    // desktop application whose first-run setup is handled in the browser.
    profile.setUserName("用户");
    profile.setAiName("Karen");
    profile.setPersonality("温柔");

    std::error_code error;
    std::filesystem::create_directories("data", error);

    if (error)
    {
        std::cerr << "无法创建 data 目录" << std::endl;
        return false;
    }

    std::ofstream saveProfile("data/profile.json");

    if (!saveProfile)
    {
        std::cerr << "无法保存 profile.json" << std::endl;
        return false;
    }

    saveProfile << profile.toJson().dump(4);
    saveProfile.close();

    if (!saveProfile)
    {
        std::cerr << "写入 profile.json 失败" << std::endl;
        return false;
    }

    return true;
}
