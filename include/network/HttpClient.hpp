#pragma once

#include <string>

struct HttpResult
{
    bool transportOk = false;
    long statusCode = 0;
    std::string body;
    std::string error;
};

class HttpClient
{
public:
    // 发起带 Bearer API Key 的 GET 请求，并返回 HTTP 状态码。
    HttpResult get(
        const std::string& url,
        const std::string& apiKey
    );

    std::string post(
        const std::string& url,
        const std::string& body,
        const std::string& apiKey
    );
};
