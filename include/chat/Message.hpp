#pragma once

#include <string>
#include <nlohmann/json.hpp>

struct Message
{
    std::string role;

    std::string content;

    nlohmann::json toJson() const ;

    static Message fromJson(const nlohmann::json& data);
};
