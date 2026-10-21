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

// All user data lives outside the installation directory, under:
// %LOCALAPPDATA%\Karen\data
//
// On first read, this helper imports an existing legacy data/<file> from the
// working directory (or its parent) if the new AppData file does not exist.
// The legacy file is deliberately left untouched as a recovery copy.
namespace KarenPaths
{
    namespace fs = std::filesystem;

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

        // Older builds used a working-directory-relative "data" folder.
        // Check both the current directory and its parent, covering launches
        // from either the install folder or a build subdirectory.
        const fs::path workingDirectory = fs::current_path(error);
        if (error)
        {
            return destination;
        }

        const std::vector<fs::path> legacyDirectories = {
            workingDirectory / L"data",
            workingDirectory.parent_path() / L"data"
        };

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

            // If migration cannot copy the old file, still allow the caller
            // to read it. Future writes always target the AppData directory.
            return legacyFile;
        }

        return destination;
    }
}
