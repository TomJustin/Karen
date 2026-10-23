cmake_minimum_required(VERSION 3.20)

# Keep the MSVC runtime consistent with the release triplet when MSVC is used.
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Karen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Embed the existing HTML/CSS/JS frontend in a native Windows window.
# webview uses WebView2 on Windows and supports MinGW-w64.
include(FetchContent)

set(WEBVIEW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WEBVIEW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WEBVIEW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(WEBVIEW_ENABLE_CHECKS OFF CACHE BOOL "" FORCE)
set(WEBVIEW_ENABLE_CLANG_FORMAT OFF CACHE BOOL "" FORCE)
set(WEBVIEW_ENABLE_CLANG_TIDY OFF CACHE BOOL "" FORCE)
set(WEBVIEW_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(WEBVIEW_INSTALL_TARGETS OFF CACHE BOOL "" FORCE)

if(MINGW)
    set(WEBVIEW_USE_COMPAT_MINGW ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
    webview
    GIT_REPOSITORY https://github.com/webview/webview.git
    GIT_TAG 0.12.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(webview)

add_executable(
    Karen
    WIN32

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
    webview::core

    # WebView2 / native Windows window / local HTTP readiness probe.
    advapi32
    ole32
    shell32
    shlwapi
    user32
    version
    winhttp

    # Windows DPAPI + Local AppData path lookup.
    crypt32
)

add_custom_command(
    TARGET Karen POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/frontend"
            "$<TARGET_FILE_DIR:Karen>/frontend"

    # Do not copy repository data (profile/chat history/memory) into a new build.
    # Runtime data remains under the current Windows user's Local AppData.
)
