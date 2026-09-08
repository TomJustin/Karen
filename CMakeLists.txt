cmake_minimum_required(VERSION 3.20)

project(Karen)

set(CMAKE_CXX_STANDAR 17)
set(CMAKE_CXX_STANDAR_REQUIRED ON)

add_executable(
    Karen
    src/main.cpp
    src/chat/Conversation.cpp
    src/network/HttpClient.cpp
    src/chat/Message.cpp
    src/model/DeepSeek.cpp
    src/user/Profile.cpp
    src/storage/ConversationStorage.cpp
    src/storage/ProfileStorage.cpp
)

target_include_directories(
    Karen
    PRIVATE
    include
)

find_package(CURL REQUIRED)

target_link_libraries(
    Karen
    PRIVATE
    CURL::libcurl
)