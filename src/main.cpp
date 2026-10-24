#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>

#include <webview/webview.h>
#include <drogon/drogon.h>

#include <stdexcept>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "storage/AppDataPaths.hpp"
#include "user/Profile.hpp"

#include "storage/ConversationStorage.hpp"
#include "storage/ProfileStorage.hpp"
#include "storage/MemoryStorage.hpp"

#include "model/ModelFactory.hpp"
#include "model/ModelConfig.hpp"

#include "memory/MemoryExtractor.hpp"
#include "server/ChatServer.hpp"

namespace
{
    constexpr INTERNET_PORT kKarenPort = 8080;
    constexpr int kStartupAttempts = 80;
    constexpr auto kStartupRetryDelay = std::chrono::milliseconds(250);

    std::wstring toWide(const std::string& text)
    {
        if (text.empty())
        {
            return {};
        }

        int length = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            nullptr,
            0
        );

        UINT codePage = CP_UTF8;
        DWORD flags = MB_ERR_INVALID_CHARS;

        if (length <= 0)
        {
            codePage = CP_ACP;
            flags = 0;
            length = MultiByteToWideChar(
                codePage,
                flags,
                text.data(),
                static_cast<int>(text.size()),
                nullptr,
                0
            );
        }

        if (length <= 0)
        {
            return L"Karen 遇到了一个错误。";
        }

        std::wstring result(static_cast<std::size_t>(length), L'\0');

        MultiByteToWideChar(
            codePage,
            flags,
            text.data(),
            static_cast<int>(text.size()),
            result.data(),
            length
        );

        return result;
    }

    void showError(const std::string& message)
    {
        const std::wstring wideMessage = toWide(message);
        MessageBoxW(
            nullptr,
            wideMessage.c_str(),
            L"Karen",
            MB_OK | MB_ICONERROR
        );
    }

    void showWebViewError(const std::string& detail)
    {
        std::wstring message =
            L"Karen 无法创建桌面窗口。\n\n"
            L"请确认已安装 Microsoft Edge WebView2 Runtime，然后重试。\n"
            L"如果问题仍然存在，请检查下面的错误信息：\n\n";

        message += toWide(detail);
        message += L"\n\n是否打开 WebView2 Runtime 官方下载页面？";

        const int choice = MessageBoxW(
            nullptr,
            message.c_str(),
            L"Karen 桌面窗口启动失败",
            MB_YESNO | MB_ICONWARNING
        );

        if (choice == IDYES)
        {
            ShellExecuteW(
                nullptr,
                L"open",
                L"https://developer.microsoft.com/microsoft-edge/webview2/",
                nullptr,
                nullptr,
                SW_SHOWNORMAL
            );
        }
    }

    // Returns the HTTP status code when anything is listening on the local
    // Karen port, or zero when no HTTP response can be obtained.
    DWORD probeKarenEndpoint(DWORD timeoutMilliseconds = 250)
    {
        HINTERNET session = WinHttpOpen(
            L"Karen Desktop",
            WINHTTP_ACCESS_TYPE_NO_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (session == nullptr)
        {
            return 0;
        }

        WinHttpSetTimeouts(
            session,
            static_cast<int>(timeoutMilliseconds),
            static_cast<int>(timeoutMilliseconds),
            static_cast<int>(timeoutMilliseconds),
            static_cast<int>(timeoutMilliseconds)
        );

        HINTERNET connection = WinHttpConnect(
            session,
            L"127.0.0.1",
            kKarenPort,
            0
        );

        HINTERNET request = nullptr;
        DWORD statusCode = 0;

        if (connection != nullptr)
        {
            request = WinHttpOpenRequest(
                connection,
                L"GET",
                L"/api/config/status",
                nullptr,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0
            );
        }

        if (request != nullptr &&
            WinHttpSendRequest(
                request,
                WINHTTP_NO_ADDITIONAL_HEADERS,
                0,
                WINHTTP_NO_REQUEST_DATA,
                0,
                0,
                0
            ) &&
            WinHttpReceiveResponse(request, nullptr))
        {
            DWORD statusSize = sizeof(statusCode);

            WinHttpQueryHeaders(
                request,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &statusCode,
                &statusSize,
                WINHTTP_NO_HEADER_INDEX
            );
        }

        if (request != nullptr)
        {
            WinHttpCloseHandle(request);
        }

        if (connection != nullptr)
        {
            WinHttpCloseHandle(connection);
        }

        WinHttpCloseHandle(session);
        return statusCode;
    }

    int runKaren()
    {
        // Resolve runtime resources beside Karen.exe irrespective of the
        // directory from which Windows launched the application.
        std::filesystem::current_path(
            KarenPaths::getExecutableDirectory()
        );

        // Do not attach a second GUI to another process already using the
        // fixed local API port.
        if (probeKarenEndpoint() != 0)
        {
            showError(
                "Karen 的本地端口 8080 已被占用。\n\n"
                "请先关闭已经运行的 Karen，或确认没有其他程序占用该端口，"
                "然后重新启动。"
            );
            return 1;
        }

        Profile profile;
        ConversationStorage conversationStorage;
        ProfileStorage profileStorage;
        MemoryStorage memoryStorage;

        Conversation conversation =
            conversationStorage.loadConversation();

        if (!profileStorage.loadProfile(profile) &&
            !profileStorage.setupProfile(profile))
        {
            throw std::runtime_error("无法初始化 Karen 的个人资料。");
        }

        MemoryStore memoryStore =
            memoryStorage.loadMemories();

        ModelConfig config;
        // First-run API Key setup remains inside the existing frontend.
        config.loadApiKey();

        const std::size_t contextMessageCount = 20;
        std::unique_ptr<AIModel> model =
            ModelFactory::createModel(config, contextMessageCount);

        MemoryExtractor extractor(*model);

        ChatServer server(
            conversation,
            profile,
            memoryStore,
            *model,
            config,
            conversationStorage,
            memoryStorage,
            extractor
        );

        std::atomic<bool> serverFinished{false};
        std::mutex serverErrorMutex;
        std::string serverError;

        std::thread serverThread([&]()
        {
            try
            {
                server.start();
            }
            catch (const std::exception& error)
            {
                std::lock_guard<std::mutex> lock(serverErrorMutex);
                serverError = error.what();
            }
            catch (...)
            {
                std::lock_guard<std::mutex> lock(serverErrorMutex);
                serverError = "后端因未知错误退出。";
            }

            serverFinished.store(true);
        });

        bool backendReady = false;

        for (int attempt = 0; attempt < kStartupAttempts; ++attempt)
        {
            if (serverFinished.load())
            {
                break;
            }

            if (probeKarenEndpoint() == 200)
            {
                backendReady = true;
                break;
            }

            std::this_thread::sleep_for(kStartupRetryDelay);
        }

        if (!backendReady)
        {
            // Make sure a partially started Drogon event loop is stopped
            // before its dependent C++ objects are destroyed.
            drogon::app().quit();

            if (serverThread.joinable())
            {
                serverThread.join();
            }

            {
                std::lock_guard<std::mutex> lock(serverErrorMutex);

                if (!serverError.empty())
                {
                    showError(
                        "Karen 后端启动失败：\n" + serverError
                    );
                }
                else
                {
                    showError(
                        "Karen 后端未能在 20 秒内启动。\n\n"
                        "请检查端口 8080 是否被占用，然后重试。"
                    );
                }
            }

            return 1;
        }

        int exitCode = 0;

        try
        {
            webview::webview window(false, nullptr);
            window.set_title("Karen");
            window.set_size(1120, 760, WEBVIEW_HINT_NONE);
            window.navigate("http://127.0.0.1:8080/");
            window.run();
        }
        catch (const webview::exception& error)
        {
            showWebViewError(error.what());
            exitCode = 1;
        }
        catch (const std::exception& error)
        {
            showError(
                "Karen 桌面窗口发生错误：\n" + std::string(error.what())
            );
            exitCode = 1;
        }

        // Closing the window also stops the local backend. Join it before
        // destroying ChatServer and the objects it references.
        drogon::app().quit();

        if (serverThread.joinable())
        {
            serverThread.join();
        }

        return exitCode;
    }
}

int WINAPI WinMain(
    HINSTANCE,
    HINSTANCE,
    LPSTR,
    int
)
{
    try
    {
        return runKaren();
    }
    catch (const std::exception& error)
    {
        showError("Karen 启动失败：\n" + std::string(error.what()));
        return 1;
    }
    catch (...)
    {
        showError("Karen 启动失败：未知错误。");
        return 1;
    }
}
