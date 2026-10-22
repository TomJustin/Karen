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

const apiKeyOverlay = document.getElementById("api-key-overlay");
const apiKeyForm = document.getElementById("api-key-form");
const apiKeyInput = document.getElementById("api-key-input");
const apiKeyStatus = document.getElementById("api-key-status");
const apiKeySaveButton = document.getElementById("api-key-save");
const apiKeyCancelButton = document.getElementById("api-key-cancel");
const apiKeyTitle = document.getElementById("api-key-title");

const DEFAULT_PROFILE = Object.freeze({
    userName: "用户",
    aiName: "Karen",
    personality: "温柔、耐心，交流自然。在日常聊天时轻松亲切，遇到需要分析的问题时认真、清晰地回答。"
});

let hasApiKey = false;
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
    aiNameStatus.textContent = aiName + " 在线";
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
        const profile = await requestProfile();
        fillProfileForm(profile);
        applyProfileToInterface(profile);
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

profileApiKeyButton.addEventListener("click", () => {
    openApiKeyDialog(!hasApiKey);
});

apiKeyForm.addEventListener("submit", saveApiKey);
apiKeyCancelButton.addEventListener("click", closeApiKeyDialog);

settingsButton.addEventListener("click", openProfileDialog);

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

requestProfile()
    .then((profile) => {
        fillProfileForm(profile);
        applyProfileToInterface(profile);
    })
    .catch((error) => {
        console.error("读取 Karen 个人资料失败：", error);
    });

checkApiKeyStatus();
