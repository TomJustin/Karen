#include "network/HttpClient.hpp"

#include <curl/curl.h>

static size_t WriteCallback(
    void* content,
    size_t size,
    size_t nmemb,
    void* userp
)
{
    const size_t totalSize = size * nmemb;
    auto* response = static_cast<std::string*>(userp);

    response->append(
        static_cast<char*>(content),
        totalSize
    );

    return totalSize;
}

HttpResult HttpClient::get(
    const std::string& url,
    const std::string& apiKey
)
{
    HttpResult response;

    CURL* curl = curl_easy_init();

    if (curl == nullptr)
    {
        response.error = "无法初始化 HTTP 客户端";
        return response;
    }

    const std::string authHeader = "Authorization: Bearer " + apiKey;
    curl_slist* headers = nullptr;

    headers = curl_slist_append(headers, authHeader.c_str());

    if (headers == nullptr)
    {
        response.error = "无法创建 HTTP 请求头";
        curl_easy_cleanup(curl);
        return response;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

    // Avoid an onboarding screen hanging indefinitely on a network problem.
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 20000L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    const CURLcode result = curl_easy_perform(curl);

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);

    if (result == CURLE_OK)
    {
        response.transportOk = true;
    }
    else
    {
        // Store only the transport error, never the authorization header.
        response.error = curl_easy_strerror(result);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}

std::string HttpClient::post(
    const std::string& url,
    const std::string& body,
    const std::string& apiKey
)
{
    const std::string authHeader = "Authorization: Bearer " + apiKey;
    std::string response;

    CURL* curl = curl_easy_init();

    if (curl == nullptr)
    {
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 60000L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    curl_slist* headers = nullptr;
    headers = curl_slist_append(
        headers,
        "Content-Type: application/json"
    );

    if (headers != nullptr)
    {
        curl_slist* withAuth = curl_slist_append(
            headers,
            authHeader.c_str()
        );

        if (withAuth != nullptr)
        {
            headers = withAuth;
        }
        else
        {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return "";
        }
    }
    else
    {
        curl_easy_cleanup(curl);
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    const CURLcode result = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK)
    {
        return "";
    }

    return response;
}
