#include "storage/ProfileStorage.hpp"
#include "storage/AppDataPaths.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

bool ProfileStorage::loadProfile(Profile& profile)
{
    std::ifstream file(
        KarenPaths::getDataFilePathForRead("profile.json"),
        std::ios::binary
    );

    if (!file)
    {
        return false;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    if (content.empty())
    {
        return false;
    }

    const nlohmann::json profileData =
        nlohmann::json::parse(content);

    profile.fromJson(profileData);
    return true;
}

bool ProfileStorage::saveProfile(const Profile& profile)
{
    std::ofstream file(
        KarenPaths::getDataFilePath("profile.json"),
        std::ios::binary | std::ios::trunc
    );

    if (!file)
    {
        std::cerr << "无法打开 profile.json 进行写入" << std::endl;
        return false;
    }

    file << profile.toJson().dump(4);
    file.close();

    if (!file)
    {
        std::cerr << "写入 profile.json 失败" << std::endl;
        return false;
    }

    return true;
}

bool ProfileStorage::setupProfile(Profile& profile)
{
    // First launch initializes a usable profile; users can customize it
    // from the settings dialog without needing console input.
    profile.setUserName("用户");
    profile.setAiName("Karen");
    profile.setPersonality(
        "温柔、耐心，交流自然。在日常聊天时轻松亲切，遇到需要分析的问题时认真、清晰地回答。"
    );

    return saveProfile(profile);
}
