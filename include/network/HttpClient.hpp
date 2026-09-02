#pragma once

#include <string>
#include <vector>

class HttpClient{
    public:

    void get(
        const std::string& url
    );

    std::string post(
        const std::string& url,
        const std::string& body,
        const std::string& apiKey
    );
};