console.log("Karen 前端启动了");

const input = document.getElementById("message-input");
const sendButton = document.getElementById("send-button");
const chatMessages = document.getElementById("chat-messages");

const settingsButton = document.getElementById("settings-button");
const apiKeyOverlay = document.getElementById("api-key-overlay");
const apiKeyForm = document.getElementById("api-key-form");
const apiKeyInput = document.getElementById("api-key-input");
const apiKeyStatus = document.getElementById("api-key-status");
const apiKeySaveButton = document.getElementById("api-key-save");
const apiKeyCancelButton = document.getElementById("api-key-cancel");
const apiKeyTitle = document.getElementById("api-key-title");

let hasApiKey = false;
let setupRequired = false;

function openApiKeyDialog(required) {
    setupRequired = required;

    apiKeyTitle.textContent = required
        ? "欢迎使用 Karen"
        : "配置 DeepSeek API Key";

    apiKeyCancelButton.hidden = required;
    apiKeyInput.value = "";
    apiKeyStatus.textContent = "";

    apiKeyOverlay.hidden = false;
    apiKeyInput.focus();
}

function closeApiKeyDialog() {
    if (setupRequired) return;

    apiKeyOverlay.hidden = true;
    apiKeyInput.value = "";
    apiKeyStatus.textContent = "";
}

async function checkApiKeyStatus() {
    try {
        const response = await fetch("/api/config/status", {
            method: "GET",
            cache: "no-store"
        });

        const data = await response.json();

        if (!response.ok || !data.success) {
            throw new Error(data.message || "无法读取配置状态");
        }

        hasApiKey = Boolean(data.configured);

        if (!hasApiKey) {
            openApiKeyDialog(true);
        }
    } catch (error) {
        // Keep the setup dialog visible; do not expose the chat UI as usable
        // when the backend cannot tell whether a key has been configured.
        openApiKeyDialog(true);
        apiKeyStatus.textContent =
            "暂时无法读取配置状态。请确认 Karen 后端已启动，然后重试。";
        console.error("读取 Karen 配置状态失败：", error);
    }
}

async function saveApiKey(event) {
    event.preventDefault();

    const apiKey = apiKeyInput.value.trim();

    if (!apiKey) {
        apiKeyStatus.textContent = "请输入 API Key。";
        apiKeyInput.focus();
        return;
    }

    apiKeySaveButton.disabled = true;
    apiKeyCancelButton.disabled = true;
    apiKeyStatus.textContent = "正在验证 API Key…";

    try {
        const response = await fetch("/api/config/api-key", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({ apiKey })
        });

        const data = await response.json();

        if (!response.ok || !data.success) {
            throw new Error(data.message || "API Key 验证失败");
        }

        hasApiKey = true;
        setupRequired = false;
        apiKeyStatus.textContent = "验证成功，配置已保存。";
        apiKeyInput.value = "";

        // Close only after the server has validated and persisted the key.
        apiKeyOverlay.hidden = true;
    } catch (error) {
        apiKeyStatus.textContent =
            error.message || "请求失败，请检查网络后重试。";
    } finally {
        apiKeySaveButton.disabled = false;
        apiKeyCancelButton.disabled = false;
    }
}

async function sendMessage() {
    const message = input.value.trim();

    if (!message || !hasApiKey) return;

    const userMessage = document.createElement("p");
    userMessage.classList.add("user-message");
    userMessage.textContent = message;
    chatMessages.appendChild(userMessage);

    input.value = "";
    sendButton.disabled = true;

    try {
        const response = await fetch("/api/chat", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({ message })
        });

        const data = await response.json();

        if (!response.ok || !data.success) {
            throw new Error(data.message || "请求失败");
        }

        const karenMessage = document.createElement("p");
        karenMessage.classList.add("karen-message");
        karenMessage.textContent = data.message;
        chatMessages.appendChild(karenMessage);
    } catch (error) {
        console.error(error);

        const errorMessage = document.createElement("p");
        errorMessage.classList.add("karen-message");
        errorMessage.textContent =
            error.message || "无法连接 Karen 后端，请检查服务器状态。";
        chatMessages.appendChild(errorMessage);
    } finally {
        sendButton.disabled = false;
        chatMessages.scrollTop = chatMessages.scrollHeight;
    }
}

apiKeyForm.addEventListener("submit", saveApiKey);

apiKeyCancelButton.addEventListener("click", closeApiKeyDialog);

settingsButton.addEventListener("click", () => {
    openApiKeyDialog(false);
});

apiKeyOverlay.addEventListener("keydown", (event) => {
    if (event.key === "Escape") {
        if (setupRequired) {
            event.preventDefault();
        } else {
            closeApiKeyDialog();
        }
    }
});

sendButton.addEventListener("click", sendMessage);

input.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !event.isComposing) {
        sendMessage();
    }
});

checkApiKeyStatus();
