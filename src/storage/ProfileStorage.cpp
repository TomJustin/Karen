#include "storage/ProfileStorage.hpp"
#include "storage/AppDataPaths.hpp"

#include <filesystem>
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

bool ProfileStorage::setupProfile(Profile& profile)
{
    // First launch must not block waiting for console input: Karen is a
    // desktop application whose first-run setup is handled in the browser.
    profile.setUserName("用户");
    profile.setAiName("Karen");
    profile.setPersonality("温柔");

    std::ofstream saveProfile(
        KarenPaths::getDataFilePath("profile.json"),
        std::ios::binary | std::ios::trunc
    );

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
