console.log("Karen 前端启动了");

const input = document.getElementById("message-input");
const sendButton = document.getElementById("send-button");
const chatMessages = document.getElementById("chat-messages");

const settingsButton = document.getElementById("settings-button");
const aiNameBrand = document.getElementById("ai-name-brand");
const aiNameHeader = document.getElementById("ai-name-header");
const aiNameStatus = document.getElementById("ai-name-status");
const aiAvatarBrand = document.getElementById("ai-avatar-brand");
const aiAvatarHeader = document.getElementById("ai-avatar-header");

const profileOverlay = document.getElementById("profile-overlay");
const profileForm = document.getElementById("profile-form");
const profileUserName = document.getElementById("profile-user-name");
const profileAiName = document.getElementById("profile-ai-name");
const profilePersonality = document.getElementById("profile-personality");
const profilePersonalityCount = document.getElementById("profile-personality-count");
const profileStatus = document.getElementById("profile-status");
const profileSaveButton = document.getElementById("profile-save");
const profileResetButton = document.getElementById("profile-reset");
const profileCloseButton = document.getElementById("profile-close");
const profileApiKeyButton = document.getElementById("profile-api-key");
const clearMemoryButton = document.getElementById("clear-memory-button");

const apiKeyOverlay = document.getElementById("api-key-overlay");
const apiKeyForm = document.getElementById("api-key-form");
const apiKeyInput = document.getElementById("api-key-input");
const apiKeyStatus = document.getElementById("api-key-status");
const apiKeySaveButton = document.getElementById("api-key-save");
const apiKeyCancelButton = document.getElementById("api-key-cancel");
const apiKeyTitle = document.getElementById("api-key-title");

const modelProviderSelect = document.getElementById("model-provider-select");
const modelNameInput = document.getElementById("model-name-input");
const modelSaveButton = document.getElementById("model-save-button");
const modelStatus = document.getElementById("model-status");

const DEFAULT_PROFILE = Object.freeze({
    userName: "用户",
    aiName: "Karen",
    personality: "温柔、耐心，交流自然。在日常聊天时轻松亲切，遇到需要分析的问题时认真、清晰地回答。"
});

let modelConfigured = false;
let modelReady = false;
let activeModelProvider = "deepseek";
let selectedModelProvider = "deepseek";
const modelDrafts = {
    deepseek: "deepseek-flash",
    ollama: "qwen3:8b"
};
let setupRequired = false;

function setProfileStatus(message, isError = false) {
    profileStatus.textContent = message;
    profileStatus.classList.toggle("error", isError);
}

function updatePersonalityCount() {
    profilePersonalityCount.textContent =
        profilePersonality.value.length + " / 4000";
}

function fillProfileForm(profile) {
    profileUserName.value = profile.userName || DEFAULT_PROFILE.userName;
    profileAiName.value = profile.aiName || DEFAULT_PROFILE.aiName;
    profilePersonality.value =
        profile.personality || DEFAULT_PROFILE.personality;
    updatePersonalityCount();
}

function applyProfileToInterface(profile) {
    const aiName = (profile.aiName || DEFAULT_PROFILE.aiName).trim();
    const avatarText = [...aiName][0] || "K";

    document.title = "Karen — " + aiName;
    aiNameBrand.textContent = aiName;
    aiNameHeader.textContent = aiName;
    aiNameStatus.textContent = modelReady
        ? aiName + " 在线"
        : (activeModelProvider === "ollama"
            ? "Ollama 未就绪"
            : "等待配置 API Key");
    aiAvatarBrand.textContent = avatarText;
    aiAvatarHeader.textContent = avatarText;
    input.placeholder = "和 " + aiName + " 说点什么...";
}

async function requestProfile() {
    const response = await fetch("/api/profile", {
        method: "GET",
        cache: "no-store"
    });

    const data = await response.json();

    if (!response.ok || !data.success || !data.profile) {
        throw new Error(data.message || "无法读取个人资料");
    }

    return data.profile;
}

async function loadProfileSettings() {
    profileSaveButton.disabled = true;
    setProfileStatus("正在读取当前设置…");

    try {
        const [profile, modelData] = await Promise.all([
            requestProfile(),
            requestModelStatus()
        ]);
        fillProfileForm(profile);
        applyProfileToInterface(profile);
        updateModelStatus(modelData);
        setProfileStatus("");
    } catch (error) {
        setProfileStatus(
            error.message || "读取设置失败，请确认 Karen 后端已启动。",
            true
        );
    } finally {
        profileSaveButton.disabled = false;
    }
}

function openProfileDialog() {
    profileOverlay.hidden = false;
    setProfileStatus("");
    void loadProfileSettings();
}

function closeProfileDialog() {
    profileOverlay.hidden = true;
    setProfileStatus("");
}

function openApiKeyDialog(required) {
    setupRequired = required;

    apiKeyTitle.textContent = required
        ? "欢迎使用 Karen"
        : "配置 DeepSeek API Key";

    // A user without a DeepSeek key must still be able to close this dialog
    // and switch to the local Ollama provider in settings.
    apiKeyCancelButton.hidden = false;
    apiKeyInput.value = "";
    apiKeyStatus.textContent = "";

    apiKeyOverlay.hidden = false;
    apiKeyInput.focus();
}

function closeApiKeyDialog() {
    setupRequired = false;
    apiKeyOverlay.hidden = true;
    apiKeyInput.value = "";
    apiKeyStatus.textContent = "";
}

function updateModelStatus(data) {
    activeModelProvider = data.provider || activeModelProvider;
    selectedModelProvider = activeModelProvider;

    modelProviderSelect.value = activeModelProvider;
    modelNameInput.value = data.model || modelDrafts[activeModelProvider] || "";
    modelDrafts[activeModelProvider] = modelNameInput.value;

    modelConfigured = Boolean(data.configured);
    modelReady = Boolean(data.ready);
    profileApiKeyButton.hidden = activeModelProvider !== "deepseek";

    modelStatus.textContent = data.message || "";
    modelStatus.classList.toggle("error", !data.ready);
    modelStatus.classList.toggle("success", Boolean(data.ready));

    aiNameStatus.textContent = data.ready
        ? aiNameHeader.textContent + " 在线"
        : (activeModelProvider === "ollama"
            ? "Ollama 未就绪"
            : "等待配置 API Key");
}

async function requestModelStatus() {
    const response = await fetch("/api/config/status", {
        method: "GET",
        cache: "no-store"
    });

    const data = await response.json();

    if (!response.ok || !data.success) {
        throw new Error(data.message || "无法读取模型配置");
    }

    return data;
}

async function checkModelStatus() {
    try {
        const data = await requestModelStatus();
        updateModelStatus(data);

        if (activeModelProvider === "deepseek" && !modelConfigured) {
            openApiKeyDialog(true);
        }
    } catch (error) {
        modelConfigured = false;
        openApiKeyDialog(true);
        apiKeyStatus.textContent =
            "暂时无法读取模型配置。请确认 Karen 后端已启动，然后重试。";
        modelStatus.textContent = error.message || "无法读取模型配置。";
        modelStatus.classList.add("error");
        console.error("读取 Karen 模型配置失败：", error);
    }
}

async function saveModelSettings() {
    const provider = modelProviderSelect.value;
    const model = modelNameInput.value.trim();

    if (!model) {
        modelStatus.textContent = "请输入模型名称。";
        modelStatus.classList.add("error");
        modelNameInput.focus();
        return;
    }

    modelSaveButton.disabled = true;
    modelStatus.textContent = "正在保存并切换模型…";
    modelStatus.classList.remove("error", "success");

    try {
        const response = await fetch("/api/model", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({ provider, model })
        });

        const data = await response.json();

        if (!response.ok || !data.success) {
            throw new Error(data.message || "模型切换失败");
        }

        updateModelStatus(data);

        if (provider === "deepseek" && !modelConfigured) {
            openApiKeyDialog(true);
        }
    } catch (error) {
        modelStatus.textContent = error.message || "模型切换失败。";
        modelStatus.classList.add("error");
        modelStatus.classList.remove("success");
    } finally {
        modelSaveButton.disabled = false;
    }
}

function handleModelProviderChange() {
    modelDrafts[selectedModelProvider] = modelNameInput.value.trim() ||
        modelDrafts[selectedModelProvider];

    selectedModelProvider = modelProviderSelect.value;
    modelNameInput.value = modelDrafts[selectedModelProvider] ||
        (selectedModelProvider === "ollama" ? "qwen3:8b" : "deepseek-flash");

    profileApiKeyButton.hidden = selectedModelProvider !== "deepseek";
    modelStatus.textContent = "选择已修改。点击“保存并切换”后生效。";
    modelStatus.classList.remove("error", "success");
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

        setupRequired = false;
        apiKeyStatus.textContent = "验证成功，配置已保存。";
        apiKeyInput.value = "";

        // Close only after the server has validated and persisted the key.
        apiKeyOverlay.hidden = true;
        void checkModelStatus();
    } catch (error) {
        apiKeyStatus.textContent =
            error.message || "请求失败，请检查网络后重试。";
    } finally {
        apiKeySaveButton.disabled = false;
        apiKeyCancelButton.disabled = false;
    }
}

async function saveProfile(event) {
    event.preventDefault();

    const profile = {
        userName: profileUserName.value.trim(),
        aiName: profileAiName.value.trim(),
        personality: profilePersonality.value.trim()
    };

    if (!profile.userName || !profile.aiName || !profile.personality) {
        setProfileStatus("请填写用户称呼、AI 名字和性格描述。", true);
        return;
    }

    profileSaveButton.disabled = true;
    profileResetButton.disabled = true;
    profileApiKeyButton.disabled = true;
    profileCloseButton.disabled = true;
    setProfileStatus("正在保存设置…");

    try {
        const response = await fetch("/api/profile", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(profile)
        });

        const data = await response.json();

        if (!response.ok || !data.success || !data.profile) {
            throw new Error(data.message || "保存设置失败");
        }

        fillProfileForm(data.profile);
        applyProfileToInterface(data.profile);
        setProfileStatus("设置已保存。");
        closeProfileDialog();
    } catch (error) {
        setProfileStatus(
            error.message || "保存失败，请检查后端或本机数据目录权限。",
            true
        );
    } finally {
        profileSaveButton.disabled = false;
        profileResetButton.disabled = false;
        profileApiKeyButton.disabled = false;
        profileCloseButton.disabled = false;
    }
}

async function clearMemoryAndConversation() {
    const confirmed = window.confirm(
        "确定要清空 Karen 的长期记忆和当前对话吗？此操作不可撤销。你的个性化设置和 API Key 不会被删除。"
    );
    if (!confirmed) return;

    clearMemoryButton.disabled = true;
    sendButton.disabled = true;
    setProfileStatus("正在清空记忆与对话…");
    try {
        const response = await fetch("/api/memory/reset", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({})
        });
        const data = await response.json();
        if (!response.ok || !data.success) {
            throw new Error(data.message || "清空记忆失败");
        }
        chatMessages.replaceChildren();
        input.value = "";
        setProfileStatus("已清空长期记忆与当前对话，可以重新开始。");
    } catch (error) {
        setProfileStatus(error.message || "清空失败，请确认 Karen 正在运行后重试。", true);
    } finally {
        clearMemoryButton.disabled = false;
        sendButton.disabled = false;
    }
}

async function sendMessage() {
    const message = input.value.trim();

    if (!message || !modelConfigured || sendButton.disabled) return;

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

profileForm.addEventListener("submit", saveProfile);
profileCloseButton.addEventListener("click", closeProfileDialog);

profileOverlay.addEventListener("click", (event) => {
    if (event.target === profileOverlay) {
        closeProfileDialog();
    }
});

profileOverlay.addEventListener("keydown", (event) => {
    if (event.key === "Escape") {
        closeProfileDialog();
    }
});

profileResetButton.addEventListener("click", () => {
    fillProfileForm(DEFAULT_PROFILE);
    setProfileStatus("已填入默认内容；点击“保存设置”后才会生效。");
});

profilePersonality.addEventListener("input", updatePersonalityCount);

document.querySelectorAll(".personality-preset").forEach((button) => {
    button.addEventListener("click", () => {
        profilePersonality.value = button.dataset.personality || "";
        updatePersonalityCount();
        profilePersonality.focus();
        setProfileStatus("模板已填入，可以继续修改后保存。");
    });
});

modelProviderSelect.addEventListener("change", handleModelProviderChange);
modelSaveButton.addEventListener("click", saveModelSettings);

profileApiKeyButton.addEventListener("click", () => {
    openApiKeyDialog(
        activeModelProvider === "deepseek" && !modelConfigured
    );
});

apiKeyForm.addEventListener("submit", saveApiKey);
apiKeyCancelButton.addEventListener("click", closeApiKeyDialog);

settingsButton.addEventListener("click", openProfileDialog);
clearMemoryButton.addEventListener("click", clearMemoryAndConversation);

apiKeyOverlay.addEventListener("keydown", (event) => {
    if (event.key === "Escape") {
        closeApiKeyDialog();
    }
});

sendButton.addEventListener("click", sendMessage);

input.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !event.isComposing) {
        sendMessage();
    }
});

requestProfile()
    .then((profile) => {
        // Keep the visible name in sync without overwriting fields while the
        // user may already be editing the settings dialog.
        applyProfileToInterface(profile);
    })
    .catch((error) => {
        console.error("读取 Karen 个人资料失败：", error);
    });

checkModelStatus();
