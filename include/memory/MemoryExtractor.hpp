#pragma once

#include <string>

#include "model/AIModel.hpp"

struct MemoryCandidate
{
    bool shouldRemember;
    std::string category;
    std::string content;
};

class MemoryExtractor
{
public:
    MemoryExtractor(AIModel& model);

    MemoryCandidate extract(
        const std::string& userMessage
    );

private:
    AIModel& model;
};