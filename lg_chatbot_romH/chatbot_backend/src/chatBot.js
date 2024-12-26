"use strict";
// // Define an interface for your memory item
// interface MemoryItem {
//     key: string;
//     value: {
//         content: string;
//         context: string;
//     };
// }
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
// import { AIMessage, BaseMessage, HumanMessage, SystemMessage } from "@langchain/core/messages";
// import { LangGraphRunnableConfig, MemorySaver, StateGraph, END } from "@langchain/langgraph";
// import { ToolNode } from "@langchain/langgraph/prebuilt";
// import { ChatOpenAI } from "@langchain/openai";
// import { InMemoryStore } from "@langchain/langgraph";
// import upsertMemoryTool from "./tools/upsertMemoryTool";
// import { SYSTEM_PROMPT } from "./prompts/prompts";
// import { ConfigurationAnnotation, ensureConfiguration } from "./configuration/configuration";
// import allProductsTool from "./tools/allProductsTool";
// import { loadMemory, saveMemory } from "./memoryStorage";
// import { DynamicStructuredTool } from "@langchain/core/tools";
// export type PrevMessages = {
//     id: string;
//     role: "user" | "assistant";
//     content: string;
// };
// const platformKeywords = ['pc', 'xbox', 'playstation', 'ps4', 'ps5', 'nintendo switch'];
// function detectPlatformKeyword(message: string): string | undefined {
//     const lowerCaseMessage = message.toLowerCase();
//     const platform = platformKeywords.find(keyword => lowerCaseMessage.includes(keyword));
//     if (platform) {
//         console.log("Platform detected:", platform);
//     }
//     return platform;
// }
// const chatBot = async (humanMessage: string, prevMessages: PrevMessages[]) => {
//     const StateAnnotation = ConfigurationAnnotation;
//     // Define the tools for the agent to use
//     const tools: DynamicStructuredTool<any>[] = [allProductsTool, upsertMemoryTool];
//     const toolNode = new ToolNode(tools);
//     const model = new ChatOpenAI({
//         model: "gpt-4o-mini",
//         temperature: 0,
//         cache: true,
//     }).bindTools(tools);
//     // Define the function that determines whether to continue or not
//     function shouldContinue(state: typeof StateAnnotation.State) {
//         const messages = state.messages;
//         const lastMessage = messages[messages.length - 1] as AIMessage;
//         if (lastMessage.tool_calls?.length) {
//             return "tools";
//         }
//         return "end";
//     }
//     // Define the function that calls the model
//     async function callModel(state: typeof StateAnnotation.State, config: LangGraphRunnableConfig) {
//         const store = config.store;
//         const userId = config?.configurable?.user_id;
//         if (!store) {
//             throw new Error("store is required when compiling the graph");
//         }
//         if (!userId) {
//             throw new Error("userId is required in the config");
//         }
//         let preferredPlatform: string | undefined;
//         try {
//             const memories = loadMemory();
//             preferredPlatform = memories[userId]?.find((mem: MemoryItem) => mem.key === "platform_preference")?.value?.content;
//             console.log("Loaded preferred platform from memory:", preferredPlatform);
//         } catch (error) {
//             console.error("Error loading memory:", error);
//         }
//         const configurable = ensureConfiguration(config);
//         const lastMessageContent = state.messages[state.messages.length - 1]?.content;
//         let messageText: string | undefined;
//         if (typeof lastMessageContent === 'string') {
//             messageText = lastMessageContent;
//         } else if (Array.isArray(lastMessageContent)) {
//             const textPart = lastMessageContent.find(item => typeof item === 'string');
//             if (typeof textPart === 'string') {
//                 messageText = textPart;
//             }
//         }
//         const detectedPlatform = messageText ? detectPlatformKeyword(messageText) : undefined;
//         if (detectedPlatform) {
//             try {
//                 await upsertMemoryTool.invoke({
//                     content: detectedPlatform,
//                     context: "User mentioned platform preference.",
//                     memoryId: "platform_preference"
//                 }, config);
//                 preferredPlatform = detectedPlatform; // Update for the current turn
//                 console.log("Successfully updated platform preference in memory.");
//             } catch (error) {
//                 console.error("Error updating memory:", error);
//             }
//         }
//         const formatted = preferredPlatform ? `\n<platform_preference>\n${preferredPlatform}\n</platform_preference>` : "";
//         const sys = configurable.systemPrompt
//             .replace("{user_info}", formatted)
//             .replace("{time}", new Date().toISOString());
//         const result = await model.invoke([{ role: "system", content: sys }, ...state.messages]);
//         return { messages: [result] };
//     }
//     function routeMessage(state: typeof StateAnnotation.State): "tools" | typeof END {
//         const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
//         if (lastMessage.tool_calls?.length) {
//             return "tools";
//         }
//         return END;
//     }
//     const workflow = new StateGraph(StateAnnotation)
//         .addNode("agent", callModel)
//         .addNode("tools", toolNode)
//         .addEdge("__start__", "agent")
//         .addConditionalEdges("agent", shouldContinue, {
//             tools: "tools",
//             end: END,
//         })
//         .addEdge("tools", "agent");
//     const checkpointer = new MemorySaver();
//     const inMemoryStore = new InMemoryStore();
//     const app = workflow.compile({
//         checkpointer,
//         store: inMemoryStore,
//     });
//     const history = (prevMessages ?? [])
//         .filter(({ role, content }) => role && content)
//         .map(({ role, content }) => (role === "user" ? new HumanMessage(content) : new AIMessage(content)));
//     const messages = [
//         new SystemMessage(SYSTEM_PROMPT),
//         ...history,
//         new HumanMessage(humanMessage),
//     ];
//     const finalState = await app.invoke(
//         { messages: messages },
//         { configurable: { thread_id: "42", user_id: "42" } }
//     );
//     return finalState.messages[finalState.messages.length - 1].content;
// };
// export default chatBot;
// import { AIMessage, BaseMessage, HumanMessage, SystemMessage } from "@langchain/core/messages";
// import { LangGraphRunnableConfig, MemorySaver, StateGraph, END } from "@langchain/langgraph";
// import { ToolNode } from "@langchain/langgraph/prebuilt";
// import { ChatOpenAI } from "@langchain/openai";
// import upsertMemoryTool from "./tools/upsertMemoryTool";
// import { SYSTEM_PROMPT } from "./prompts/prompts";
// import { ConfigurationAnnotation, ensureConfiguration } from "./configuration/configuration";
// import allProductsTool from "./tools/allProductsTool";
// import { DynamicStructuredTool,  Tool } from "@langchain/core/tools";
// import { pool } from "./db/db"; // Import your database pool
// import { InMemoryStore } from "@langchain/langgraph";
// export type PrevMessages = {
//     id: string;
//     role: "user" | "assistant";
//     content: string;
// };
// const platformKeywords = ['pc', 'xbox', 'playstation', 'ps4', 'ps5', 'nintendo switch'];
// function detectPlatformKeyword(message: string): string | undefined {
//     const lowerCaseMessage = message.toLowerCase();
//     const platform = platformKeywords.find(keyword => lowerCaseMessage.includes(keyword));
//     if (platform) {
//         console.log("Platform detected:", platform);
//     }
//     return platform;
// }
// const chatBot = async (humanMessage: string, prevMessages: PrevMessages[]) => {
//     const StateAnnotation = ConfigurationAnnotation;
//     // Define the tools for the agent to use
//     const tools: DynamicStructuredTool<any>[] = [allProductsTool, upsertMemoryTool];
//     const toolNode = new ToolNode(tools);
//     const model = new ChatOpenAI({
//         model: "gpt-4o-mini",
//         temperature: 0,
//         cache: true,
//     }).bindTools(tools);
//     // Define the function that determines whether to continue or not
//     function shouldContinue(state: typeof StateAnnotation.State) {
//         const messages = state.messages;
//         const lastMessage = messages[messages.length - 1] as AIMessage;
//         if (lastMessage.tool_calls?.length) {
//             return "tools";
//         }
//         return "end";
//     }
//     // Define the function that calls the model
//     async function callModel(state: typeof StateAnnotation.State, config: LangGraphRunnableConfig) {
//         const store = config.store;
//         const userId = config?.configurable?.user_id;
//         if (!store) {
//             throw new Error("store is required when compiling the graph");
//         }
//         if (!userId) {
//             throw new Error("userId is required in the config");
//         }
//          const configurable = ensureConfiguration(config);
//         let formatted = "";
//         const sys = configurable.systemPrompt
//             .replace("{user_info}", formatted)
//             .replace("{time}", new Date().toISOString());
//          const result = await model.invoke([{ role: "system", content: sys }, ...state.messages]);
//         return { messages: [result] };
//     }
//     async function storeMemory(state: typeof StateAnnotation.State, config: LangGraphRunnableConfig): Promise<{ messages: BaseMessage[] }> {
//         const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
//         const toolCalls = lastMessage.tool_calls || [];
//         console.log("toolCalls in storeMemory:", toolCalls)
//         let savedMemories: BaseMessage[] = [];
//         if (toolCalls.length > 0) {
//             savedMemories = await Promise.all(
//                 toolCalls.map(async (tc) => {
//                     const tool = tools.find((tool) => tool.name === tc.name);
//                     if (!tool) {
//                        return null
//                     }
//                     try {
//                          return await tool.invoke(tc.args, config)
//                     } catch (error) {
//                         console.error(`Error invoking tool ${tool.name}:`, error);
//                          return null;
//                     }
//                 }),
//             );
//         }
//          return { messages: savedMemories.filter(Boolean) } as { messages: BaseMessage[] };
//      }
//     function routeMessage(state: typeof StateAnnotation.State): "tools" | typeof END {
//         const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
//         if (lastMessage.tool_calls?.length) {
//             return "tools";
//         }
//         return END;
//     }
//    const workflow = new StateGraph(StateAnnotation)
//         .addNode("agent", callModel)
//         .addNode("tools", toolNode)
//          .addNode("store_memory", storeMemory)
//         .addEdge("__start__", "agent")
//         .addConditionalEdges("agent", shouldContinue, {
//           tools: "tools",
//           end: END,
//         })
//          .addEdge("tools", "store_memory")
//         .addEdge("store_memory", "agent");
//     const checkpointer = new MemorySaver();
//     const inMemoryStore = new InMemoryStore();
//     const app = workflow.compile({
//         checkpointer,
//         store: inMemoryStore,
//     });
//     const history = (prevMessages ?? [])
//         .filter(({ role, content }) => role && content)
//         .map(({ role, content }) => (role === "user" ? new HumanMessage(content) : new AIMessage(content)));
//     const messages = [
//         new SystemMessage(SYSTEM_PROMPT),
//         ...history,
//         new HumanMessage(humanMessage),
//     ];
//     const finalState = await app.invoke(
//         { messages: messages },
//         { configurable: { thread_id: "42", user_id: "42" } }
//     );
//     return finalState.messages[finalState.messages.length - 1].content;
// };
// export default chatBot;
const messages_1 = require("@langchain/core/messages");
const langgraph_1 = require("@langchain/langgraph");
const prebuilt_1 = require("@langchain/langgraph/prebuilt");
const openai_1 = require("@langchain/openai");
const upsertMemoryTool_1 = __importDefault(require("./tools/upsertMemoryTool"));
const prompts_1 = require("./prompts/prompts");
const configuration_1 = require("./configuration/configuration");
const allProductsTool_1 = __importDefault(require("./tools/allProductsTool"));
const langgraph_2 = require("@langchain/langgraph");
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
            var _a, _b;
            const store = config.store;
            const userId = (_a = config === null || config === void 0 ? void 0 : config.configurable) === null || _a === void 0 ? void 0 : _a.user_id;
            if (!store) {
                throw new Error("store is required when compiling the graph");
            }
            if (!userId) {
                throw new Error("userId is required in the config");
            }
            const configurable = (0, configuration_1.ensureConfiguration)(config);
            let formatted = "";
            const lastMessageContent = (_b = state.messages[state.messages.length - 1]) === null || _b === void 0 ? void 0 : _b.content;
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
                    formatted = `\n<platform_preference>\n${detectedPlatform}\n</platform_preference>`;
                    console.log("Updated the platform to:", detectedPlatform);
                }
                catch (error) {
                    console.error("Failed to update platform preference", error);
                }
            }
            const sys = configurable.systemPrompt
                .replace("{user_info}", formatted)
                .replace("{time}", new Date().toISOString());
            const result = yield model.invoke([{ role: "system", content: sys }, ...state.messages]);
            return { messages: [result] };
        });
    }
    function storeMemory(state, config) {
        return __awaiter(this, void 0, void 0, function* () {
            const lastMessage = state.messages[state.messages.length - 1];
            const toolCalls = lastMessage.tool_calls || [];
            console.log("toolCalls in storeMemory:", toolCalls);
            let savedMemories = [];
            if (toolCalls.length > 0) {
                savedMemories = yield Promise.all(toolCalls.map((tc) => __awaiter(this, void 0, void 0, function* () {
                    const tool = tools.find((tool) => tool.name === tc.name);
                    if (!tool) {
                        return null;
                    }
                    try {
                        return yield tool.invoke(tc.args, config);
                    }
                    catch (error) {
                        console.error(`Error invoking tool ${tool.name}:`, error);
                        return null;
                    }
                })));
            }
            return { messages: savedMemories.filter(Boolean) };
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
        .addNode("store_memory", storeMemory)
        .addEdge("__start__", "agent")
        .addConditionalEdges("agent", shouldContinue, {
        tools: "tools",
        end: langgraph_1.END,
    })
        .addEdge("tools", "store_memory")
        .addEdge("store_memory", "agent");
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
