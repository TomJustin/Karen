#include "server/ChatServer.hpp"
#include "network/HttpClient.hpp"
#include "storage/ProfileStorage.hpp"

#include <drogon/drogon.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

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

    bool containsDisallowedControlCharacters(
        const std::string& value,
        bool allowLineBreaks
    )
    {
        return std::any_of(
            value.begin(),
            value.end(),
            [allowLineBreaks](unsigned char ch)
            {
                if (ch == 0x7f)
                {
                    return true;
                }

                if (ch >= 0x20)
                {
                    return false;
                }

                return !(allowLineBreaks &&
                    (ch == '\n' || ch == '\r' || ch == '\t'));
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
     * 3. 读取个人资料
     * ==============================
     */

    drogon::app().registerHandler(
        "/api/profile",
        [this](
            const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&
            )>&& callback
        )
        {
            Profile currentProfile;
            {
                std::lock_guard<std::mutex> lock(profileMutex);
                currentProfile = profile;
            }

            Json::Value response;
            response["success"] = true;
            response["profile"]["userName"] = currentProfile.getUserName();
            response["profile"]["aiName"] = currentProfile.getAiName();
            response["profile"]["personality"] = currentProfile.getPersonality();

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->addHeader("Cache-Control", "no-store");
            callback(resp);
        },
        {drogon::Get}
    );


    /*
     * ==============================
     * 4. 更新个人资料
     * ==============================
     */

    drogon::app().registerHandler(
        "/api/profile",
        [this](
            const drogon::HttpRequestPtr& req,
            std::function<void(
                const drogon::HttpResponsePtr&
            )>&& callback
        )
        {
            // Only accept state-changing requests from Karen's local UI.
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
                !requestJson->isMember("userName") ||
                !(*requestJson)["userName"].isString() ||
                !requestJson->isMember("aiName") ||
                !(*requestJson)["aiName"].isString() ||
                !requestJson->isMember("personality") ||
                !(*requestJson)["personality"].isString())
            {
                Json::Value response;
                response["success"] = false;
                response["message"] =
                    "请求必须包含 userName、aiName 和 personality 字符串";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k400BadRequest);
                callback(resp);
                return;
            }

            const std::string userName =
                trimWhitespace((*requestJson)["userName"].asString());
            const std::string aiName =
                trimWhitespace((*requestJson)["aiName"].asString());
            const std::string personality =
                trimWhitespace((*requestJson)["personality"].asString());

            if (userName.empty() ||
                aiName.empty() ||
                personality.empty() ||
                userName.size() > 1024 ||
                aiName.size() > 1024 ||
                personality.size() > 12000 ||
                containsDisallowedControlCharacters(userName, false) ||
                containsDisallowedControlCharacters(aiName, false) ||
                containsDisallowedControlCharacters(personality, true))
            {
                Json::Value response;
                response["success"] = false;
                response["message"] =
                    "请填写有效的姓名和性格描述，并确保内容没有超出长度限制";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(response);

                resp->setStatusCode(drogon::k400BadRequest);
                callback(resp);
                return;
            }

            Profile updatedProfile;
            {
                std::lock_guard<std::mutex> lock(profileMutex);
                updatedProfile = profile;
                updatedProfile.setUserName(userName);
                updatedProfile.setAiName(aiName);
                updatedProfile.setPersonality(personality);

                ProfileStorage storage;
                if (!storage.saveProfile(updatedProfile))
                {
                    Json::Value response;
                    response["success"] = false;
                    response["message"] =
                        "设置未能保存到本机，请检查应用数据目录的写入权限";

                    auto resp =
                        drogon::HttpResponse::newHttpJsonResponse(response);

                    resp->setStatusCode(drogon::k500InternalServerError);
                    callback(resp);
                    return;
                }

                profile = updatedProfile;
            }

            Json::Value response;
            response["success"] = true;
            response["message"] = "个性化设置已保存";
            response["profile"]["userName"] = updatedProfile.getUserName();
            response["profile"]["aiName"] = updatedProfile.getAiName();
            response["profile"]["personality"] = updatedProfile.getPersonality();

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->addHeader("Cache-Control", "no-store");
            callback(resp);
        },
        {drogon::Post}
    );


    /*
     * ==============================
     * Clear long-term memory and the current conversation
     * ==============================
     */
    drogon::app().registerHandler(
        "/api/memory/reset",
        [this](
            const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback
        )
        {
            const std::string origin = req->getHeader("origin");
            if (!origin.empty() && origin != "http://127.0.0.1:8080" &&
                origin != "http://localhost:8080")
            {
                Json::Value response;
                response["success"] = false;
                response["message"] = "请求来源不被允许";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                resp->setStatusCode(drogon::k403Forbidden);
                callback(resp);
                return;
            }

            std::lock_guard<std::mutex> stateLock(stateMutex);
            try
            {
                MemoryStore emptyMemoryStore;
                Conversation emptyConversation;
                memoryStorage.saveMemories(emptyMemoryStore);
                conversationStorage.saveConversation(emptyConversation);
                memoryStore.clear();
                while (conversation.size() > 0)
                {
                    conversation.removeMessage(
                        static_cast<std::size_t>(conversation.size() - 1)
                    );
                }

                Json::Value response;
                response["success"] = true;
                response["message"] = "长期记忆与当前对话已清空";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                resp->addHeader("Cache-Control", "no-store");
                callback(resp);
            }
            catch (const std::exception& error)
            {
                Json::Value response;
                response["success"] = false;
                response["message"] = std::string("清空失败：") + error.what();
                auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
            }
        },
        {drogon::Post}
    );


    /*
     * ==============================
     * 5. POST /api/chat
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
            std::lock_guard<std::mutex> stateLock(stateMutex);
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
                        drogon::HttpResponse::newHttpJsonResponse(response);

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
                        drogon::HttpResponse::newHttpJsonResponse(response);

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
                        drogon::HttpResponse::newHttpJsonResponse(response);

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

                Profile profileSnapshot;
                {
                    std::lock_guard<std::mutex> lock(profileMutex);
                    profileSnapshot = profile;
                }

                std::string answer =
                    model.chat(
                        conversation,
                        profileSnapshot,
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
                 * 7. 提取并整理长期记忆
                 * ==============================
                 *
                 * Memory extraction is best-effort and runs after the chat
                 * has been saved. A memory-provider or disk error must not
                 * turn a successful answer into a failed chat response.
                 */

                try
                {
                    const MemoryCandidate candidate =
                        extractor.extract(
                            input,
                            memoryStore.getMemories()
                        );

                    if (candidate.shouldRemember)
                    {
                        Memory memory;
                        memory.category = candidate.category;
                        memory.content = candidate.content;

                        bool memoryChanged = false;

                        if (candidate.targetIndices.empty())
                        {
                            memoryChanged = memoryStore.addMemory(memory);
                        }
                        else
                        {
                            std::vector<std::size_t> duplicateIndices(
                                candidate.targetIndices.begin() + 1,
                                candidate.targetIndices.end()
                            );

                            memoryChanged = memoryStore.updateMemory(
                                candidate.targetIndices.front(),
                                memory,
                                duplicateIndices
                            );
                        }

                        if (memoryChanged)
                        {
                            memoryStorage.saveMemories(memoryStore);
                        }
                    }
                }
                catch (const std::exception& memoryError)
                {
                    std::cerr
                        << "长期记忆处理失败，本次聊天仍正常返回："
                        << memoryError.what()
                        << std::endl;
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
                    drogon::HttpResponse::newHttpJsonResponse(response);

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
                    drogon::HttpResponse::newHttpJsonResponse(response);

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
