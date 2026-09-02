#include "network/HttpClient.hpp"
#include <curl/curl.h>

static size_t WriteCallback(
    void* content,
    size_t nmemb,
    size_t size,
    void* userp
){
    size_t totalSize = size * nmemb;

    std::string* response = 
    static_cast<std::string*>(userp);

    response->append(
        static_cast<char*>(content),
        totalSize
    );

    return totalSize;
}

std::string HttpClient::post(
    const std::string& url,
    const std::string& body,
    const std::string& apiKey
)
{
    std::string authHeader = "Authorization: Bearer ";
    authHeader += apiKey;

    std::string response;

    CURL* curl = curl_easy_init();

    if (!curl)
    {
        return "";
    }

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POST,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDS,
        body.c_str()
    );

    struct curl_slist* headers = nullptr;

    headers = curl_slist_append(
        headers,
        "Content-Type: application/json"
    );

    headers = curl_slist_append(
        headers,
        authHeader.c_str() 
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        WriteCallback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response
    );

    CURLcode result =
        curl_easy_perform(curl);

    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

    if (result != CURLE_OK)
    {
        return "";
    }

    return response;
}