console.log("Karen 前端启动了");

const input = document.getElementById("message-input");

const sendButton = document.getElementById("send-button");

const chatMessages = document.getElementById("chat-messages");


async function sendMessage() {
    const message = input.value.trim();

    if (!message) return;

    const userMessage = document.createElement("p");

    userMessage.classList.add("user-message");

    userMessage.textContent = message;

    chatMessages.appendChild(userMessage);

    input.value = "";

    try {
        const response = await fetch("/api/chat", {
            method: "POST",

            headers: {
                "Content-Type": "application/json"
            },

            body: JSON.stringify({
                message: message
            })
        });

        const data = await response.json();

        if (!data.success) {
            throw new Error(
                data.message || "请求失败"
            );
        }

        const karenMessage =
            document.createElement("p");

        karenMessage.classList.add(
            "karen-message"
        );

        karenMessage.textContent =
            data.message;

        chatMessages.appendChild(
            karenMessage
        );
    }
    catch (error) {
        console.error(error);

        const errorMessage =
            document.createElement("p");

        errorMessage.classList.add(
            "karen-message"
        );

        errorMessage.textContent =
            "抱歉，连接不到 Karen 后端。";

        chatMessages.appendChild(
            errorMessage
        );
    }

    chatMessages.scrollTop =
        chatMessages.scrollHeight;
}


// 点击发送按钮
sendButton.addEventListener("click", function () {

    sendMessage();

});


// 按 Enter 发送
input.addEventListener("keydown", function (event) {

    if (event.key === "Enter") {

        sendMessage();

    }

});