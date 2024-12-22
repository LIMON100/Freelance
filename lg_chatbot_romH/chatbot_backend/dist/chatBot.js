"use strict";
// import {AIMessage, BaseMessage, HumanMessage, SystemMessage} from "@langchain/core/messages";
// import { LangGraphRunnableConfig, MemorySaver, StateGraph, END } from "@langchain/langgraph";
// import {ToolNode} from "@langchain/langgraph/prebuilt";
// import {ChatOpenAI} from "@langchain/openai";
// import { InMemoryStore } from "@langchain/langgraph";
// import upsertMemoryTool from "./tools/upsertMemoryTool";
// import { SYSTEM_PROMPT } from "./prompts/prompts";
// import { ConfigurationAnnotation, ensureConfiguration } from "./configuration/configuration";
// import priceByProductTool from "./tools/priceByProductTool";
// import allProductsTool from "./tools/allProductsTool";
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
// export type PrevMessages = {
//     id: string;
//     role: "user" | "assistant";
//     content: string;
// }
// const chatBot = async (humanMessage: string, prevMessages: PrevMessages[]) => {
//     const StateAnnotation = ConfigurationAnnotation;
//     // Define the tools for the agent to use
//     const tools = [
//         allProductsTool,
//         upsertMemoryTool,
//         // priceByProductTool
//     ];
//     const toolNode = new ToolNode(tools);
//     const model = new ChatOpenAI({
//         model: "gpt-4o-mini",
//         temperature: 0,
//         cache: true,
//     }).bindTools(tools);
// // Define the function that determines whether to continue or not
// // We can extract the state typing via `StateAnnotation.State`
//     function shouldContinue(state: typeof StateAnnotation.State) {
//         const messages = state.messages;
//         const lastMessage = messages[messages.length - 1] as AIMessage;
//         // If the LLM makes a tool call, then we route to the "tools" node
//         if (lastMessage.tool_calls?.length) {
//             return "tools";
//         }
//         // Otherwise, we stop (reply to the user)
//         return "__end__";
//     }
//     // Define the function that calls the model
//     async function callModel(
//       state: typeof StateAnnotation.State,
//       config: LangGraphRunnableConfig
//     ) {
//         const store = config.store;
//         const userId = config.configurable?.user_id;
//         if (!store) {
//             if (!store) {
//                 throw new Error("store is required when compiling the graph");
//             }
//         }
//         if (!userId) {
//             throw new Error("userId is required in the config");
//         }
//         const memories = await store.search(["12", "memories"]);
//         // TODO: 1. Check the console.log bellow.
//         //  You can easily invoke a upsertMemory tool by message in a chat "my platform is pc".
//         //  After the second iteration, the memories are gone.
//         //  Do they need to be stored in a database?
//         console.log("memories callModel", memories);
//         let formatted =
//           memories
//             ?.map((mem) => `[${mem.key}]: ${JSON.stringify(mem.value)}`)
//             ?.join("\n") || "";
//         if (formatted) {
//             formatted = `\n<memories>\n${formatted}\n</memories>`;
//         }
//         const configurable = ensureConfiguration(config);
//         const sys = configurable.systemPrompt
//           .replace("{user_info}", formatted)
//           .replace("{time}", new Date().toISOString());
//         // const messages = state.messages;
//         //
//         // const response = await model.invoke(messages);
//         //
//         // // We return a list, because this will get added to the existing list
//         // return { messages: [response] };
//         const result = await model.invoke(
//           [{ role: "system", content: sys }, ...state.messages],
//         );
//         return { messages: [result] };
//     }
//     async function storeMemory(
//       state: typeof StateAnnotation.State,
//     ): Promise<{ messages: BaseMessage[] }> {
//         const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
//         const toolCalls = lastMessage.tool_calls || [];
//         const savedMemories = await Promise.all(
//           toolCalls.map(async (tc) => {
//               return await upsertMemoryTool.invoke(tc); // TODO: 3. I'm not sure about this functionality
//           }),
//         );
//         console.log("savedMemories:", savedMemories);
//         return { messages: savedMemories };
//     }
//     function routeMessage(
//       state: typeof StateAnnotation.State,
//     ): "store_memory" | typeof END {
//         const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
//         if (lastMessage.tool_calls?.length) {
//             return "store_memory";
//         }
//         return END;
//     }
//     // Define a new graph
//     const workflow = new StateGraph(StateAnnotation)
//         .addNode("agent", callModel)
//         // .addNode("store_memory", storeMemory)
//         .addNode("tools", toolNode)
//         .addEdge("__start__", "agent")
//         // .addConditionalEdges("agent", routeMessage, {
//         //   store_memory: "store_memory",
//         //   [END]: END,
//         // })
//         // .addEdge("store_memory", "agent")
//         .addConditionalEdges("agent", shouldContinue)
//         .addEdge("tools", "agent");
//     // Initialize memory to persist state between graph runs
//     const checkpointer = new MemorySaver();
//     const inMemoryStore = new InMemoryStore();
//     // const userId = "12";
//     // const namespaceForMemory = [userId, "memories"];
//     // const memoryId = uuid4();
//     // const memory = { platform_preference: "pc" };
//     // await inMemoryStore.put(namespaceForMemory, memoryId, memory);
//     // Finally, we compile it!
//     // This compiles it into a LangChain Runnable.
//     // Note that we're (optionally) passing the memory when compiling the graph
//     const app = workflow.compile({
//         checkpointer,
//         store: inMemoryStore,
//     });
//     const history = (prevMessages ?? [])
//         .filter(({ role, content }) => role && content)
//         .map(({ role, content }) =>
//             role === 'user' ? new HumanMessage(content) : new AIMessage(content)
//         );
//     const messages = [
//         new SystemMessage(
//           SYSTEM_PROMPT // TODO: 2. Should it be stored here? Perhaps it needs to be dynamically updated instead
//         ),
//         ...history,
//         new HumanMessage(humanMessage),
//     ];
//     // Use the Runnable
//     const finalState = await app.invoke(
//         { messages: messages },
//         { configurable: { thread_id: "42", user_id: "42" } },
//     );
//     return finalState.messages[finalState.messages.length - 1].content;
// }
// export default chatBot;
// copilot-workable
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
// export type PrevMessages = {
//   id: string;
//   role: "user" | "assistant";
//   content: string;
// };
// const chatBot = async (humanMessage: string, prevMessages: PrevMessages[]) => {
//   const StateAnnotation = ConfigurationAnnotation;
//   // Define the tools for the agent to use
//   const tools = [allProductsTool, upsertMemoryTool];
//   const toolNode = new ToolNode(tools);
//   const model = new ChatOpenAI({
//     model: "gpt-4o-mini",
//     temperature: 0,
//     cache: true,
//   }).bindTools(tools);
//   // Define the function that determines whether to continue or not
//   function shouldContinue(state: typeof StateAnnotation.State) {
//     const messages = state.messages;
//     const lastMessage = messages[messages.length - 1] as AIMessage;
//     if (lastMessage.tool_calls?.length) {
//       return "tools";
//     }
//     return "end";
//   }
//   // Define the function that calls the model
//   async function callModel(state: typeof StateAnnotation.State, config: LangGraphRunnableConfig) {
//     const store = config.store;
//     const userId = config?.configurable?.user_id;
//     if (!store) {
//       throw new Error("store is required when compiling the graph");
//     }
//     if (!userId) {
//       throw new Error("userId is required in the config");
//     }
//     const memories = loadMemory();
//     console.log("memories callModel", memories);
//     let formatted = memories[userId]
//       ? `\n<memories>\n${JSON.stringify(memories[userId], null, 2)}\n</memories>`
//       : "";
//     const configurable = ensureConfiguration(config);
//     const sys = configurable.systemPrompt
//       .replace("{user_info}", formatted)
//       .replace("{time}", new Date().toISOString());
//     const result = await model.invoke([{ role: "system", content: sys }, ...state.messages]);
//     return { messages: [result] };
//   }
//   async function storeMemory(state: typeof StateAnnotation.State): Promise<{ messages: BaseMessage[] }> {
//     const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
//     const toolCalls = lastMessage.tool_calls || [];
//     const savedMemories = await Promise.all(
//       toolCalls.map(async (tc) => {
//         return await upsertMemoryTool.invoke(tc);
//       })
//     );
//     const userId = state.configurable?.user_id;
//     const memories = loadMemory();
//     memories[userId] = savedMemories;
//     saveMemory(memories);
//     console.log("savedMemories:", savedMemories);
//     return { messages: savedMemories };
//   }
//   function routeMessage(state: typeof StateAnnotation.State): "store_memory" | typeof END {
//     const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
//     if (lastMessage.tool_calls?.length) {
//       return "store_memory";
//     }
//     return END;
//   }
//   const workflow = new StateGraph(StateAnnotation)
//     .addNode("agent", callModel)
//     .addNode("store_memory", storeMemory)
//     .addNode("tools", toolNode)
//     .addEdge("start", "agent")
//     .addConditionalEdges("agent", shouldContinue)
//     .addEdge("tools", "agent");
//   const checkpointer = new MemorySaver();
//   const inMemoryStore = new InMemoryStore();
//   const app = workflow.compile({
//     checkpointer,
//     store: inMemoryStore,
//   });
//   const history = (prevMessages ?? [])
//     .filter(({ role, content }) => role && content)
//     .map(({ role, content }) => (role === "user" ? new HumanMessage(content) : new AIMessage(content)));
//   const messages = [
//     new SystemMessage(SYSTEM_PROMPT),
//     ...history,
//     new HumanMessage(humanMessage),
//   ];
//   const finalState = await app.invoke(
//     { messages: messages },
//     { configurable: { thread_id: "42", user_id: "42" } }
//   );
//   return finalState.messages[finalState.messages.length - 1].content;
// };
// export default chatBot;
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
            var _a;
            const store = config.store;
            const userId = (_a = config === null || config === void 0 ? void 0 : config.configurable) === null || _a === void 0 ? void 0 : _a.user_id;
            if (!store) {
                throw new Error("store is required when compiling the graph");
            }
            if (!userId) {
                throw new Error("userId is required in the config");
            }
            const memories = (0, memoryStorage_1.loadMemory)();
            console.log("memories callModel", memories);
            let formatted = memories[userId]
                ? `\n<memories>\n${JSON.stringify(memories[userId], null, 2)}\n</memories>`
                : "";
            const configurable = (0, configuration_1.ensureConfiguration)(config);
            const sys = configurable.systemPrompt
                .replace("{user_info}", formatted)
                .replace("{time}", new Date().toISOString());
            const result = yield model.invoke([{ role: "system", content: sys }, ...state.messages]);
            return { messages: [result] };
        });
    }
    function storeMemory(state) {
        return __awaiter(this, void 0, void 0, function* () {
            const lastMessage = state.messages[state.messages.length - 1];
            const toolCalls = lastMessage.tool_calls || [];
            const savedMemories = yield Promise.all(toolCalls.map((tc) => __awaiter(this, void 0, void 0, function* () {
                return yield upsertMemoryTool_1.default.invoke(tc);
            })));
            // const userId = state.config.configurable?.user_id;
            const userId = state.userId;
            const memories = (0, memoryStorage_1.loadMemory)();
            memories[userId] = savedMemories;
            (0, memoryStorage_1.saveMemory)(memories);
            console.log("savedMemories:", savedMemories);
            return { messages: savedMemories };
        });
    }
    function routeMessage(state) {
        var _a;
        const lastMessage = state.messages[state.messages.length - 1];
        if ((_a = lastMessage.tool_calls) === null || _a === void 0 ? void 0 : _a.length) {
            return "store_memory";
        }
        return langgraph_1.END;
    }
    const workflow = new langgraph_1.StateGraph(StateAnnotation)
        .addNode("agent", callModel)
        .addNode("store_memory", storeMemory)
        .addNode("tools", toolNode)
        .addEdge("__start__", "agent")
        .addConditionalEdges("agent", shouldContinue)
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
//# sourceMappingURL=chatBot.js.map