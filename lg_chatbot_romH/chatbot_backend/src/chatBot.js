"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const messages_1 = require("@langchain/core/messages");
const langgraph_1 = require("@langchain/langgraph");
const prebuilt_1 = require("@langchain/langgraph/prebuilt");
const openai_1 = require("@langchain/openai");
const langgraph_2 = require("@langchain/langgraph");
const upsertMemoryTool_1 = __importDefault(require("./tools/upsertMemoryTool"));
const prompts_1 = require("./prompts/prompts");
const configuration_1 = require("./configuration/configuration");
const allProductsTool_1 = __importDefault(require("./tools/allProductsTool"));
const memoryStorage_1 = require("./memoryStorage");
const platformKeywords = ['pc', 'xbox', 'playstation', 'ps4', 'ps5', 'nintendo switch'];
function detectPlatformKeyword(message) {
    const lowerCaseMessage = message.toLowerCase();
    const platform = platformKeywords.find(keyword => lowerCaseMessage.includes(keyword));
    if (platform) {
        console.log("Platform detected:", platform);
    }
    return platform;
}
const chatBot = (humanMessage, prevMessages) => __awaiter(void 0, void 0, void 0, function* () {
    const StateAnnotation = configuration_1.ConfigurationAnnotation;
    // Define the tools for the agent to use
    const tools = [allProductsTool_1.default, upsertMemoryTool_1.default];
    const toolNode = new prebuilt_1.ToolNode(tools);
    const model = new openai_1.ChatOpenAI({
        model: "gpt-4o-mini",
        temperature: 0,
        cache: true,
    }).bindTools(tools);
    // Define the function that determines whether to continue or not
    function shouldContinue(state) {
        var _a;
        const messages = state.messages;
        const lastMessage = messages[messages.length - 1];
        if ((_a = lastMessage.tool_calls) === null || _a === void 0 ? void 0 : _a.length) {
            return "tools";
        }
        return "end";
    }
    // Define the function that calls the model
    function callModel(state, config) {
        return __awaiter(this, void 0, void 0, function* () {
            var _a, _b, _c, _d, _e;
            const store = config.store;
            const userId = (_a = config === null || config === void 0 ? void 0 : config.configurable) === null || _a === void 0 ? void 0 : _a.user_id;
            if (!store) {
                throw new Error("store is required when compiling the graph");
            }
            if (!userId) {
                throw new Error("userId is required in the config");
            }
            let preferredPlatform;
            try {
                const memories = (0, memoryStorage_1.loadMemory)();
                preferredPlatform = (_d = (_c = (_b = memories[userId]) === null || _b === void 0 ? void 0 : _b.find((mem) => mem.key === "platform_preference")) === null || _c === void 0 ? void 0 : _c.value) === null || _d === void 0 ? void 0 : _d.content;
                console.log("Loaded preferred platform from memory:", preferredPlatform);
            }
            catch (error) {
                console.error("Error loading memory:", error);
            }
            const configurable = (0, configuration_1.ensureConfiguration)(config);
            const lastMessageContent = (_e = state.messages[state.messages.length - 1]) === null || _e === void 0 ? void 0 : _e.content;
            let messageText;
            if (typeof lastMessageContent === 'string') {
                messageText = lastMessageContent;
            }
            else if (Array.isArray(lastMessageContent)) {
                const textPart = lastMessageContent.find(item => typeof item === 'string');
                if (typeof textPart === 'string') {
                    messageText = textPart;
                }
            }
            const detectedPlatform = messageText ? detectPlatformKeyword(messageText) : undefined;
            if (detectedPlatform) {
                try {
                    yield upsertMemoryTool_1.default.invoke({
                        content: detectedPlatform,
                        context: "User mentioned platform preference.",
                        memoryId: "platform_preference"
                    }, config);
                    preferredPlatform = detectedPlatform; // Update for the current turn
                    console.log("Successfully updated platform preference in memory.");
                }
                catch (error) {
                    console.error("Error updating memory:", error);
                }
            }
            const formatted = preferredPlatform ? `\n<platform_preference>\n${preferredPlatform}\n</platform_preference>` : "";
            const sys = configurable.systemPrompt
                .replace("{user_info}", formatted)
                .replace("{time}", new Date().toISOString());
            const result = yield model.invoke([{ role: "system", content: sys }, ...state.messages]);
            return { messages: [result] };
        });
    }
    function routeMessage(state) {
        var _a;
        const lastMessage = state.messages[state.messages.length - 1];
        if ((_a = lastMessage.tool_calls) === null || _a === void 0 ? void 0 : _a.length) {
            return "tools";
        }
        return langgraph_1.END;
    }
    const workflow = new langgraph_1.StateGraph(StateAnnotation)
        .addNode("agent", callModel)
        .addNode("tools", toolNode)
        .addEdge("__start__", "agent")
        .addConditionalEdges("agent", shouldContinue, {
        tools: "tools",
        end: langgraph_1.END,
    })
        .addEdge("tools", "agent");
    const checkpointer = new langgraph_1.MemorySaver();
    const inMemoryStore = new langgraph_2.InMemoryStore();
    const app = workflow.compile({
        checkpointer,
        store: inMemoryStore,
    });
    const history = (prevMessages !== null && prevMessages !== void 0 ? prevMessages : [])
        .filter(({ role, content }) => role && content)
        .map(({ role, content }) => (role === "user" ? new messages_1.HumanMessage(content) : new messages_1.AIMessage(content)));
    const messages = [
        new messages_1.SystemMessage(prompts_1.SYSTEM_PROMPT),
        ...history,
        new messages_1.HumanMessage(humanMessage),
    ];
    const finalState = yield app.invoke({ messages: messages }, { configurable: { thread_id: "42", user_id: "42" } });
    return finalState.messages[finalState.messages.length - 1].content;
});
exports.default = chatBot;
