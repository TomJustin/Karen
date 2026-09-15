#include "memory/MemoryExtractor.hpp"

MemoryExtractor::MemoryExtractor(AIModel& model) : model(model){}

MemoryCandidate MemoryExtractor::extract(
    const std::string& userMessage
){
    Message systemMessage;
    systemMessage.role = "system";
    systemMessage.content =
        "你是一个长期记忆提取器。"
        "请分析用户刚刚说的话，判断其中是否存在值得长期保存的信息。"
        "如果值得保存，返回 shouldRemember=true，并填写 category 和 content。"
        "如果不值得保存，返回 shouldRemember=false。"
        "你必须只返回 JSON，不要输出任何额外文字。";

    Message userMessageObject;
    userMessageObject.role = "user";
    userMessageObject.content = userMessage;

    std::vector<Message> messages;

    messages.push_back(systemMessage);
    messages.push_back(userMessageObject);
        
    std::string response = model.chat(messages);

    nlohmann::json data = nlohmann::json::parse(response);

    MemoryCandidate candidate;

    candidate.shouldRemember =
        data["shouldRemember"].get<bool>();

    if (candidate.shouldRemember)
{
    candidate.category =
        data["category"].get<std::string>();

    candidate.content =
        data["content"].get<std::string>();
}
else
{
    candidate.category = "";
    candidate.content = "";
}

    return candidate;

    }