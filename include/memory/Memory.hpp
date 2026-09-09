#pragma once

#include <string>
#include <nlohmann/json.hpp>


struct Memory{
    std::string category;

    std::string content;

    nlohmann::json toJson() const;

    static Memory fromJson(const nlohmann::json& data);
};