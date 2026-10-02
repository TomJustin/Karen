#include "server/ChatServer.hpp"

#include <drogon/drogon.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

using json = nlohmann::json;

ChatServer::ChatServer(
    Conversation& conversation,
    Profile& profile,
    MemoryStore& memoryStore,
    AIModel& model,
    ConversationStorage& conversationStorage,
    MemoryStorage& memoryStorage,
    MemoryExtractor& extractor
)
    : conversation(conversation),
      profile(profile),
      memoryStore(memoryStore),
      model(model),
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
     * 2. POST /api/chat
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