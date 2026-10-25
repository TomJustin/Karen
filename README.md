# Karen

![Version](https://img.shields.io/badge/version-1.0.1-7c6ee6)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-0078D4)
![Language](https://img.shields.io/badge/C%2B%2B-17-00599C)

**Karen 是一个运行在 Windows 桌面上的 AI 陪伴聊天应用。** 它使用 C++ 实现本地后端，以原生桌面窗口承载聊天界面，并通过 DeepSeek API 生成回复。你可以自定义 AI 的名字、性格与对你的称呼，也可以让它积累长期记忆。

项目仍在持续完善中。目前的重点是把核心聊天、个性化设置、本地数据保存和桌面运行流程做好。

## ✨ 主要功能

- **桌面窗口运行**：直接打开 Karen.exe 使用，不需要手动打开浏览器。
- **AI 对话**：通过 DeepSeek API 生成回复，需要自行提供可用的 API Key。
- **个性化设置**：自定义用户称呼、AI 名字和性格描述，让交流风格更符合自己的偏好。
- **长期记忆**：尝试从对话中提取值得保留的稳定信息，例如长期目标、兴趣和沟通偏好，并在后续对话中作为上下文使用。
- **本地保存**：在本机保存个人资料、当前对话和长期记忆，重启应用后可以继续使用。
- **重新开始**：可以清空当前对话和长期记忆，从头开始交流；个人资料与 API Key 会保留。
- **首次启动配置**：在应用内填写 API Key，应用会验证密钥后再保存，不需要手动设置系统环境变量。

## 📥 下载与运行

当前发布版本：**v1.0.1**

前往 [GitHub Releases](https://github.com/TomJustin/Karen/releases/latest) 下载 Karen-windows-x64.zip。

1. 下载并解压整个 ZIP 压缩包到一个文件夹中。
2. 双击运行 Karen.exe。
3. 首次启动时，在配置窗口中填写自己的 DeepSeek API Key。
4. 验证通过后即可开始聊天。以后启动时会读取本机已保存的配置。

这是一个 **Windows x64 便携版压缩包**，不需要安装向导。请保留解压后的整个文件夹，不要只复制 Karen.exe 到其他位置。

### 需要准备什么？

- 64 位 Windows。
- 可访问 DeepSeek API 的网络连接。
- 自己的 DeepSeek API Key。可前往 [DeepSeek 开放平台](https://platform.deepseek.com/) 查看 API 服务与账户用量。
- **Microsoft Edge WebView2 Runtime**。大多数较新的 Windows 系统已经具备该运行环境。如果 Karen 无法创建桌面窗口，可以从 [WebView2 官方页面](https://developer.microsoft.com/microsoft-edge/webview2/) 安装 Evergreen Runtime，再重新启动应用。

API 请求会产生相应的模型用量，具体额度与费用以 DeepSeek 账户和官方说明为准。

## ⚙️ 使用说明

### 个性化 AI

点击窗口右上角的齿轮按钮，可以编辑：

- **用户称呼**：Karen 应该怎样称呼你。
- **AI 名字**：你希望给 AI 起什么名字。
- **性格与相处方式**：用自然语言描述你希望 AI 的性格、表达方式和交流风格。

保存后，设置会保存在本机，并用于后续对话。

### 长期记忆与清空数据

Karen 会尝试识别对以后交流有帮助的稳定信息，并把它们作为长期记忆保存。记忆提取由 AI 完成，因此不保证每条信息都能被识别或准确归纳。

在个性化设置中，可以选择清空长期记忆与当前对话。此操作不可撤销；它不会清除你的个人资料或 API Key。

## 🗂️ 数据保存与隐私

Karen 的个人数据默认保存在当前 Windows 用户的本地应用数据目录下：

- **应用数据目录**：`%LOCALAPPDATA%\Karen\data\`，用于保存个人资料、对话记录和长期记忆。
- **API Key**：`%LOCALAPPDATA%\Karen\config\api_key.bin`，使用 Windows DPAPI 加密后保存。

需要注意：**本地保存不等于所有内容都只在本机处理。** 为了生成回复和提取长期记忆，应用会向 DeepSeek API 发送相关请求。请求可能包含当前对话内容、个性化设置以及作为上下文使用的长期记忆。请勿输入你不愿意发送给第三方 API 服务的敏感信息，并同时查看 DeepSeek 的相关服务条款与隐私说明。

Karen 的便携压缩包不包含你的 API Key、个人资料或对话数据。

## 🧰 技术栈

| 技术 | 用途 |
| --- | --- |
| C++17 | 应用后端与核心逻辑 |
| Drogon | 本地 HTTP 服务与 API 路由 |
| WebView / WebView2 | 在原生 Windows 窗口中展示聊天界面 |
| HTML、CSS、JavaScript | 聊天界面与交互 |
| libcurl | 向 DeepSeek API 发起 HTTP 请求 |
| nlohmann/json | JSON 数据处理 |
| CMake、vcpkg | 构建配置与 C++ 依赖管理 |
| GitHub Actions | 自动构建 Windows 发布包 |

## 🏗️ 项目结构

~~~text
Karen/
├── frontend/       # HTML、CSS、JavaScript 聊天界面
├── include/        # C++ 头文件
├── src/
│   ├── chat/       # 消息与对话管理
│   ├── memory/     # 长期记忆与记忆提取
│   ├── model/      # AI 模型调用与配置
│   ├── network/    # HTTP 请求封装
│   ├── server/     # 本地 HTTP 服务与 API 路由
│   ├── storage/    # 本地数据持久化
│   └── user/       # 用户资料与个性化设置
├── packaging/      # 发布包说明与依赖清单
├── .github/
│   └── workflows/  # GitHub Actions 自动构建与发布流程
└── CMakeLists.txt  # CMake 构建配置
~~~

## 🛠️ 从源码构建

项目使用 CMake 和 C++17，当前 Windows 发布流程基于 MinGW-w64、vcpkg 与 GitHub Actions。C++ 依赖包括 Drogon、libcurl 和 nlohmann/json，WebView 相关代码通过 CMake FetchContent 获取。

如果你希望自行编译，请先准备适用于 Windows x64 的 C++ 编译环境、CMake、Ninja 和 vcpkg，并按照仓库中的 CMakeLists.txt、packaging/vcpkg-dependencies.txt 及 .github/workflows/windows-release.yml 配置对应的工具链与依赖。普通用户建议直接使用 Releases 中的预编译便携包。

## 🧭 当前范围

Karen 目前主要围绕**单个持续使用的聊天空间**设计，还没有完整的多会话管理、账号系统或云端同步功能。长期记忆属于辅助能力，提取结果可能不完整，建议不要把它当作唯一的数据备份。

## 📄 许可说明

当前仓库尚未添加 LICENSE 文件。若计划将代码用于其他项目或再分发，请先联系作者确认使用许可。

---

如果你发现问题或有改进建议，欢迎通过 [GitHub Issues](https://github.com/TomJustin/Karen/issues) 反馈。
