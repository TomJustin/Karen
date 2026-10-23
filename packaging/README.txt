Karen for Windows x64 — Portable Desktop Package

Start
1. Extract the entire ZIP archive to a folder you can write to.
2. Double-click Karen.exe. Karen opens in its own desktop window; no external browser is needed.
3. On first launch, enter your own DeepSeek API Key in the setup dialog.

If the desktop window cannot start
- Karen uses Microsoft Edge WebView2 Runtime to display its interface.
- If prompted, open the official WebView2 Runtime download page and install the Evergreen Runtime:
  https://developer.microsoft.com/microsoft-edge/webview2/
- Restart Karen after installation.

Personalization
- Click the gear icon to change your preferred name, the AI's name, and its personality.
- Changes are saved locally and used in subsequent conversations.

Local data
- Encrypted API Key: %LOCALAPPDATA%\Karen\config\api_key.bin
- Profile, chat history and long-term memory: %LOCALAPPDATA%\Karen\data\

The API Key is not included in this package. An internet connection is required to reach the DeepSeek API.

To stop Karen
- Close the Karen desktop window. The local backend will stop when the window closes.

This is a portable ZIP package. It does not modify Windows environment variables and
does not include or overwrite your personal profile, chat history or memory files.
