#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <shlobj.h>

// Keep application data outside the installation directory and anchor
// executable-relative resources to the executable, not the caller's CWD.
namespace KarenPaths
{
    namespace fs = std::filesystem;

    inline fs::path getExecutableDirectory()
    {
        // GetModuleFileNameW handles Unicode Windows paths.
        std::vector<wchar_t> buffer(32768);
        const DWORD length = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size())
        );

        if (length == 0 || length >= buffer.size())
        {
            throw std::runtime_error("无法确定 Karen 程序所在目录");
        }

        return fs::path(std::wstring(buffer.data(), length)).parent_path();
    }

    inline fs::path getDataDirectory()
    {
        wchar_t localAppData[MAX_PATH] = {};

        const HRESULT result = SHGetFolderPathW(
            nullptr,
            CSIDL_LOCAL_APPDATA,
            nullptr,
            SHGFP_TYPE_CURRENT,
            localAppData
        );

        if (FAILED(result) || localAppData[0] == L'\0')
        {
            throw std::runtime_error(
                "无法获取 Windows 本地应用数据目录"
            );
        }

        const fs::path directory =
            fs::path(localAppData) / L"Karen" / L"data";

        std::error_code error;
        fs::create_directories(directory, error);

        if (error)
        {
            throw std::runtime_error(
                "无法创建 Karen 用户数据目录：" + error.message()
            );
        }

        return directory;
    }

    inline fs::path getDataFilePath(const std::string& fileName)
    {
        return getDataDirectory() / fs::path(fileName);
    }

    inline fs::path getDataFilePathForRead(const std::string& fileName)
    {
        const fs::path destination = getDataFilePath(fileName);

        std::error_code error;
        if (fs::is_regular_file(destination, error) && !error)
        {
            return destination;
        }

        // Legacy versions stored data relative to the working directory.
        // Check the caller's directory, its parent and the executable folder
        // so existing local data can be migrated after packaging changes.
        std::vector<fs::path> legacyDirectories;
        const fs::path executableDirectory = getExecutableDirectory();
        legacyDirectories.push_back(executableDirectory / L"data");

        const fs::path workingDirectory = fs::current_path(error);
        if (!error)
        {
            legacyDirectories.push_back(workingDirectory / L"data");
            legacyDirectories.push_back(
                workingDirectory.parent_path() / L"data"
            );
        }

        for (const auto& legacyDirectory : legacyDirectories)
        {
            const fs::path legacyFile =
                legacyDirectory / fs::path(fileName);

            if (legacyFile == destination)
            {
                continue;
            }

            error.clear();
            if (!fs::is_regular_file(legacyFile, error) || error)
            {
                continue;
            }

            error.clear();
            fs::copy_file(
                legacyFile,
                destination,
                fs::copy_options::none,
                error
            );

            if (!error)
            {
                return destination;
            }

            // If import fails, allow reading the old file as a fallback.
            // New writes still target the LocalAppData destination.
            return legacyFile;
        }

        return destination;
    }
}
