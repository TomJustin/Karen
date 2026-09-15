cmake_minimum_required(VERSION 3.20)

project(Karen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    Karen
    src/main.cpp
    src/chat/Conversation.cpp
    src/network/HttpClient.cpp
    src/chat/Message.cpp
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
)

target_include_directories(
    Karen
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

find_package(CURL REQUIRED)

target_link_libraries(
    Karen
    PRIVATE
    CURL::libcurl
)