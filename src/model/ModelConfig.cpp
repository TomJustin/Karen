#include "model/ModelConfig.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <system_error>
#include <utility>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <shlobj.h>
#include <wincrypt.h>

namespace
{
    std::filesystem::path getApiKeyPath()
    {
        wchar_t localAppData[MAX_PATH] = {};

        const HRESULT result = SHGetFolderPathW(
            nullptr,
            CSIDL_LOCAL_APPDATA,
            nullptr,
            SHGFP_TYPE_CURRENT,
            localAppData
        );

        if (FAILED(result))
        {
            return {};
        }

        return std::filesystem::path(localAppData)
            / L"Karen"
            / L"config"
            / L"api_key.bin";
    }

    bool loadSavedApiKey(std::string& loadedKey)
    {
        const std::filesystem::path path = getApiKeyPath();

        if (path.empty())
        {
            return false;
        }

        std::ifstream input(
            path,
            std::ios::binary | std::ios::ate
        );

        if (!input)
        {
            return false;
        }

        const std::streamoff fileSize =
            static_cast<std::streamoff>(input.tellg());

        // DPAPI output for a short API key should be tiny.
        // The limit also prevents an unexpected file from allocating huge memory.
        if (fileSize <= 0 || fileSize > 64 * 1024)
        {
            return false;
        }

        std::vector<BYTE> encrypted(
            static_cast<std::size_t>(fileSize)
        );

        input.seekg(0, std::ios::beg);
        input.read(
            reinterpret_cast<char*>(encrypted.data()),
            static_cast<std::streamsize>(encrypted.size())
        );

        if (!input)
        {
            return false;
        }

        DATA_BLOB inputBlob{};
        inputBlob.cbData = static_cast<DWORD>(encrypted.size());
        inputBlob.pbData = encrypted.data();

        DATA_BLOB outputBlob{};

        if (!CryptUnprotectData(
                &inputBlob,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                CRYPTPROTECT_UI_FORBIDDEN,
                &outputBlob))
        {
            return false;
        }

        if (outputBlob.pbData != nullptr && outputBlob.cbData > 0)
        {
            loadedKey.assign(
                reinterpret_cast<const char*>(outputBlob.pbData),
                outputBlob.cbData
            );

            SecureZeroMemory(
                outputBlob.pbData,
                outputBlob.cbData
            );
        }

        if (outputBlob.pbData != nullptr)
        {
            LocalFree(outputBlob.pbData);
        }

        return !loadedKey.empty();
    }

    bool writeEncryptedApiKey(const std::string& key)
    {
        if (key.empty() ||
            key.size() > std::numeric_limits<DWORD>::max())
        {
            return false;
        }

        const std::filesystem::path path = getApiKeyPath();

        if (path.empty())
        {
            return false;
        }

        std::error_code filesystemError;

        std::filesystem::create_directories(
            path.parent_path(),
            filesystemError
        );

        if (filesystemError)
        {
            return false;
        }

        std::vector<BYTE> plain(key.begin(), key.end());

        DATA_BLOB inputBlob{};
        inputBlob.cbData = static_cast<DWORD>(plain.size());
        inputBlob.pbData = plain.data();

        DATA_BLOB encryptedBlob{};

        const BOOL encrypted = CryptProtectData(
            &inputBlob,
            L"Karen API Key",
            nullptr,
            nullptr,
            nullptr,
            CRYPTPROTECT_UI_FORBIDDEN,
            &encryptedBlob
        );

        if (!plain.empty())
        {
            SecureZeroMemory(plain.data(), plain.size());
        }

        if (!encrypted)
        {
            return false;
        }

        std::filesystem::path tempPath = path;
        tempPath += L".tmp";

        bool writeSucceeded = false;

        {
            std::ofstream output(
                tempPath,
                std::ios::binary | std::ios::trunc
            );

            if (output)
            {
                output.write(
                    reinterpret_cast<const char*>(
                        encryptedBlob.pbData
                    ),
                    static_cast<std::streamsize>(
                        encryptedBlob.cbData
                    )
                );

                output.close();
                writeSucceeded = !output.fail();
            }
        }

        if (encryptedBlob.pbData != nullptr)
        {
            LocalFree(encryptedBlob.pbData);
        }

        if (!writeSucceeded)
        {
            std::filesystem::remove(tempPath, filesystemError);
            return false;
        }

        // Replace the old file only after the new encrypted file is complete.
        if (!MoveFileExW(
                tempPath.c_str(),
                path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            std::filesystem::remove(tempPath, filesystemError);
            return false;
        }

        return true;
    }
}

ModelConfig::ModelConfig()
    : modelName("deepseek")
{
}

bool ModelConfig::loadApiKey()
{
    std::string loadedKey;

    // Prefer the app's saved key so a developer's environment variable
    // cannot unexpectedly override the user's configured key.
    if (loadSavedApiKey(loadedKey))
    {
        {
            std::lock_guard<std::mutex> lock(apiKeyMutex);
            apiKey = loadedKey;
        }

        if (!loadedKey.empty())
        {
            SecureZeroMemory(loadedKey.data(), loadedKey.size());
        }

        return true;
    }

    // Keep support for existing installations that use the environment variable.
    const char* environmentKey = std::getenv("DEEPSEEK_API_KEY");

    if (environmentKey != nullptr && environmentKey[0] != '\0')
    {
        std::lock_guard<std::mutex> lock(apiKeyMutex);
        apiKey = environmentKey;
        return true;
    }

    std::lock_guard<std::mutex> lock(apiKeyMutex);
    apiKey.clear();
    return false;
}

std::string ModelConfig::getApiKey() const
{
    std::lock_guard<std::mutex> lock(apiKeyMutex);
    return apiKey;
}

std::string ModelConfig::getModelName() const
{
    return modelName;
}

bool ModelConfig::updateApiKey(const std::string& newApiKey)
{
    if (newApiKey.empty())
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(apiKeyMutex);
    apiKey = newApiKey;
    return true;
}

bool ModelConfig::saveApiKey(const std::string& newApiKey)
{
    if (newApiKey.empty())
    {
        return false;
    }

    // Prevent two simultaneous settings requests from racing on the temp file.
    std::lock_guard<std::mutex> persistenceLock(apiKeyPersistenceMutex);

    // Do not change the active key unless persistence succeeds.
    if (!writeEncryptedApiKey(newApiKey))
    {
        return false;
    }

    return updateApiKey(newApiKey);
}

bool ModelConfig::hasApiKey() const
{
    std::lock_guard<std::mutex> lock(apiKeyMutex);
    return !apiKey.empty();
}
