#pragma once

#include <memory>
#include <string>

#include "model/AIModel.hpp"
#include "model/ModelConfig.hpp"

class ModelFactory
{
public:

    static std::unique_ptr<AIModel> createModel(
    const ModelConfig& config,
    std::size_t contextMessageCount
);

};