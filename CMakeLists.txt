cmake_minimum_required(VERSION 3.20)

project(Karen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用 Release 构建
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(
    Karen

    src/main.cpp

    src/chat/Conversation.cpp
    src/chat/Message.cpp

    src/network/HttpClient.cpp

    src/model/DeepSeek.cpp
    src/model/ModelConfig.cpp
    src/model/ModelFactory.cpp

    src/user/Profile.cpp

    src/storage/ConversationStorage.cpp
    src/storage/ProfileStorage.cpp
    src/storage/MemoryStorage.cpp

    src/memory/Memory.cpp
    src/memory/MemoryStore.cpp
    src/memory/MemoryExtractor.cpp

    src/server/ChatServer.cpp
)

target_include_directories(
    Karen
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Drogon CONFIG REQUIRED)

target_link_libraries(
    Karen
    PRIVATE

    CURL::libcurl
    nlohmann_json::nlohmann_json
    Drogon::Drogon
)

add_custom_command(
    TARGET Karen POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/frontend"
            "$<TARGET_FILE_DIR:Karen>/frontend"

    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:Karen>/data"
)