#include "server/ChatServer.hpp"
#include "network/HttpClient.hpp"

#include <drogon/drogon.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

using json = nlohmann::json;

namespace
{
    std::string trimWhitespace(std::string value)
    {
        const auto isNotSpace = [](unsigned char ch)
        {
            return std::isspace(ch) == 0;
        };

        const auto first = std::find_if(
            value.begin(),
            value.end(),
            isNotSpace
        );

        const auto last = std::find_if(
            value.rbegin(),
            value.rend(),
            isNotSpace
        ).base();

        if (first >= last)
        {
            return {};
        }

        return std::string(first, last);
    }

    bool containsWhitespaceOrControl(const std::string& value)
    {
        return std::any_of(
            value.begin(),
            value.end(),
            [](unsigned char ch)
            {
                return std::isspace(ch) != 0 || ch < 0x20 || ch == 0x7f;
            }
        );
    }
}

ChatServer::ChatServer(
    Conversation& conversation,
    Profile& profile,
    MemoryStore& memoryStore,
    AIModel& model,
    ModelConfig& config,
    ConversationStorage& conversationStorage,
    MemoryStorage& memoryStorage,
    MemoryExtractor& extractor
)
    : conversation(conversation),
      profile(profile),
      memoryStore(memoryStore),
      model(model),
      config(config),
      conversationStorage(conversationStorage),
      memoryStorage(memoryStorage),
      extractor(extractor)
{
}

void ChatServer::start()
{
    /*
     * ==============================
     * 1. 静态文件
     * ==============================
     */

    drogon::app()
        .setDocumentRoot("./frontend");



    /*
     * ==============================
     * API Key 配置状态
     * ==============================
     */

    drogon::app().registerHandler(
        "/api/config/status",
        [this](
            const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&
            )>&& callback
        )
        {
            Json::Value response;
            response["success"] = true;
            response["configured"] = config.hasApiKey();

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->addHeader("Cache-Control", "no-store");
            callback(resp);
        },
        {drogon::Get}
    );


    /*
     * ==============================
     * 保存 / 更换 API Key
     * ==============================
     */

    drogon::app().registerHandler(
        "/api/config/api-key",
        [this](
            const drogon::HttpRequestPtr& req,
            std::function<void(
                const drogon::HttpResponsePtr&
            )>&& callback
        )
        {
            // Browsers include Origin on this JSON POST. Reject cross-site
            // pages before performing a state-changing operation.
            const std::string origin = req->getHeader("origin");

            if (!origin.empty() &&
                origin != "http://127.0.0.1:8080" &&
                origin != "http://localhost:8080")
            {
                Json::Value response;
                response["success"] = false;
                response["message"] = "请求来源不被允许";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k403Forbidden);
                callback(resp);
                return;
            }

            auto requestJson = req->getJsonObject();

            if (!requestJson ||
                !requestJson->isObject() ||
                !requestJson->isMember("apiKey") ||
                !(*requestJson)["apiKey"].isString())
            {
                Json::Value response;
                response["success"] = false;
                response["message"] = "请求必须包含字符串 apiKey";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k400BadRequest);
                callback(resp);
                return;
            }

            std::string apiKey =
                trimWhitespace((*requestJson)["apiKey"].asString());

            if (apiKey.empty() ||
                apiKey.size() > 512 ||
                containsWhitespaceOrControl(apiKey))
            {
                Json::Value response;
                response["success"] = false;
                response["message"] = "API Key 格式不正确";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k400BadRequest);
                callback(resp);
                return;
            }

            // The official GET /models endpoint lets us check authentication
            // without spending tokens on a chat completion.
            HttpClient validator;
            const HttpResult validation = validator.get(
                "https://api.deepseek.com/models",
                apiKey
            );

            if (!validation.transportOk)
            {
                Json::Value response;
                response["success"] = false;
                response["message"] =
                    "无法连接 DeepSeek API，请检查网络或代理设置后重试";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k502BadGateway);
                callback(resp);
                return;
            }

            if (validation.statusCode == 401 ||
                validation.statusCode == 403)
            {
                Json::Value response;
                response["success"] = false;
                response["message"] =
                    "API Key 无效或没有访问 DeepSeek API 的权限";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k400BadRequest);
                callback(resp);
                return;
            }

            if (validation.statusCode != 200)
            {
                Json::Value response;
                response["success"] = false;
                response["message"] =
                    "DeepSeek API 验证未成功（HTTP " +
                    std::to_string(validation.statusCode) + "）";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k502BadGateway);
                callback(resp);
                return;
            }

            // Do not accept an unrelated HTML/proxy response that happens to
            // use HTTP 200. Confirm the documented model-list response shape.
            bool validModelsResponse = false;

            try
            {
                const json modelsResponse = json::parse(validation.body);

                validModelsResponse =
                    modelsResponse.is_object() &&
                    modelsResponse.contains("object") &&
                    modelsResponse["object"].is_string() &&
                    modelsResponse["object"] == "list" &&
                    modelsResponse.contains("data") &&
                    modelsResponse["data"].is_array();
            }
            catch (const json::parse_error&)
            {
                validModelsResponse = false;
            }

            if (!validModelsResponse)
            {
                Json::Value response;
                response["success"] = false;
                response["message"] =
                    "DeepSeek API 返回了无法识别的验证响应，请检查网络或代理设置";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k502BadGateway);
                callback(resp);
                return;
            }

            // Save only after successful remote validation. On a disk/DPAPI
            // error, the previously active key remains unchanged.
            if (!config.saveApiKey(apiKey))
            {
                Json::Value response;
                response["success"] = false;
                response["message"] =
                    "API Key 验证成功，但无法安全保存到本机";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
                return;
            }

            Json::Value response;
            response["success"] = true;
            response["message"] = "API Key 验证成功并已安全保存";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->addHeader("Cache-Control", "no-store");
            callback(resp);
        },
        {drogon::Post}
    );


    /*
     * ==============================
     * 3. POST /api/chat
     * ==============================
     */

    drogon::app().registerHandler(
        "/api/chat",

        [this](
            const drogon::HttpRequestPtr& req,
            std::function<void(
                const drogon::HttpResponsePtr&
            )>&& callback
        )
        {
            if (!config.hasApiKey())
            {
                Json::Value response;
                response["success"] = false;
                response["message"] = "请先在设置中配置 DeepSeek API Key";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k503ServiceUnavailable);
                callback(resp);
                return;
            }

            const auto originalSize = conversation.size();
            bool conversationSaved = false;

            try
            {
                /*
                 * ------------------------------
                 * 获取 JSON 请求
                 * ------------------------------
                 */

                auto requestJson =
                    req->getJsonObject();

                if (!requestJson)
                {
                    Json::Value response;

                    response["success"] = false;
                    response["message"] =
                        "请求必须是 JSON";

                    auto resp =
                        drogon::HttpResponse::
                        newHttpJsonResponse(response);

                    resp->setStatusCode(
                        drogon::k400BadRequest
                    );

                    callback(resp);

                    return;
                }


                /*
                 * ------------------------------
                 * 检查 message
                 * ------------------------------
                 */

                if (!requestJson->isMember("message"))
                {
                    Json::Value response;

                    response["success"] = false;
                    response["message"] =
                        "缺少 message 字段";

                    auto resp =
                        drogon::HttpResponse::
                        newHttpJsonResponse(response);

                    resp->setStatusCode(
                        drogon::k400BadRequest
                    );

                    callback(resp);

                    return;
                }


                /*
                 * ------------------------------
                 * 获取用户消息
                 * ------------------------------
                 */

                std::string input =
                    (*requestJson)["message"].asString();

                if (input.empty())
                {
                    Json::Value response;

                    response["success"] = false;
                    response["message"] =
                        "消息不能为空";

                    auto resp =
                        drogon::HttpResponse::
                        newHttpJsonResponse(response);

                    resp->setStatusCode(
                        drogon::k400BadRequest
                    );

                    callback(resp);

                    return;
                }


                /*
                 * ==============================
                 * 3. 加入用户消息
                 * ==============================
                 */

                Message message;

                message.role = "user";
                message.content = input;

                conversation.addMessage(message);


                /*
                 * ==============================
                 * 4. 调用 AI
                 * ==============================
                 */

                std::string answer =
                    model.chat(
                        conversation,
                        profile,
                        memoryStore
                    );


                /*
                 * ==============================
                 * 5. 保存 Karen 回复
                 * ==============================
                 */

                Message assistantMessage;

                assistantMessage.role =
                    "assistant";

                assistantMessage.content =
                    answer;

                conversation.addMessage(
                    assistantMessage
                );


                /*
                 * ==============================
                 * 6. 保存聊天记录
                 * ==============================
                 */

                conversationStorage.saveConversation(
                    conversation
                );
                conversationSaved = true;

                /*
                 * ==============================
                 * 7. 提取 Memory
                 * ==============================
                 */

                MemoryCandidate candidate =
                    extractor.extract(input);

                if (candidate.shouldRemember)
                {
                    Memory memory;

                    memory.category =
                        candidate.category;

                    memory.content =
                        candidate.content;

                    memoryStore.addMemory(memory);

                    memoryStorage.saveMemories(
                        memoryStore
                    );
                }


                /*
                 * ==============================
                 * 8. 返回 JSON
                 * ==============================
                 */

                Json::Value response;

                response["success"] = true;
                response["message"] = answer;

                auto resp =
                    drogon::HttpResponse::
                    newHttpJsonResponse(response);

                callback(resp);
            }
            catch (const std::exception& e)
            {
                /*
                 * ==============================
                 * 异常处理
                 * ==============================
                 */
                if (!conversationSaved)
                {
                    while (conversation.size() > originalSize)
                    {
                        conversation.removeMessage(
                            conversation.size() - 1
                        );
                    }
                }

                Json::Value response;

                response["success"] = false;
                response["message"] = e.what();

                auto resp =
                    drogon::HttpResponse::
                    newHttpJsonResponse(response);

                resp->setStatusCode(
                    drogon::k500InternalServerError
                );

                callback(resp);
            }
        },

        {drogon::Post}
    );


    /*
     * ==============================
     * 3. 启动服务器
     * ==============================
     */

    std::cout
        << "================================="
        << std::endl;

    std::cout
        << "Karen Server Started"
        << std::endl;

    std::cout
        << "Frontend:"
        << std::endl;

    std::cout
        << "http://127.0.0.1:8080/"
        << std::endl;

    std::cout
        << "================================="
        << std::endl;


    drogon::app()
        .addListener(
            "127.0.0.1",
            8080
        )
        .run();
}