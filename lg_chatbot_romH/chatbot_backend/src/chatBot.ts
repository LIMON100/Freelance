// Define an interface for your memory item
interface MemoryItem {
    key: string;
    value: {
        content: string;
        context: string;
    };
}

import { AIMessage, BaseMessage, HumanMessage, SystemMessage } from "@langchain/core/messages";
import { LangGraphRunnableConfig, MemorySaver, StateGraph, END } from "@langchain/langgraph";
import { ToolNode } from "@langchain/langgraph/prebuilt";
import { ChatOpenAI } from "@langchain/openai";
import { InMemoryStore } from "@langchain/langgraph";
import upsertMemoryTool from "./tools/upsertMemoryTool";
import { SYSTEM_PROMPT } from "./prompts/prompts";
import { ConfigurationAnnotation, ensureConfiguration } from "./configuration/configuration";
import allProductsTool from "./tools/allProductsTool";
import { loadMemory, saveMemory } from "./memoryStorage";
import { DynamicStructuredTool } from "@langchain/core/tools";

export type PrevMessages = {
    id: string;
    role: "user" | "assistant";
    content: string;
};

const platformKeywords = ['pc', 'xbox', 'playstation', 'ps4', 'ps5', 'nintendo switch'];

function detectPlatformKeyword(message: string): string | undefined {
    const lowerCaseMessage = message.toLowerCase();
    const platform = platformKeywords.find(keyword => lowerCaseMessage.includes(keyword));
    if (platform) {
        console.log("Platform detected:", platform);
    }
    return platform;
}

const chatBot = async (humanMessage: string, prevMessages: PrevMessages[]) => {
    const StateAnnotation = ConfigurationAnnotation;

    // Define the tools for the agent to use
    const tools: DynamicStructuredTool<any>[] = [allProductsTool, upsertMemoryTool];
    const toolNode = new ToolNode(tools);

    const model = new ChatOpenAI({
        model: "gpt-4o-mini",
        temperature: 0,
        cache: true,
    }).bindTools(tools);

    // Define the function that determines whether to continue or not
    function shouldContinue(state: typeof StateAnnotation.State) {
        const messages = state.messages;
        const lastMessage = messages[messages.length - 1] as AIMessage;

        if (lastMessage.tool_calls?.length) {
            return "tools";
        }
        return "end";
    }

    // Define the function that calls the model
    async function callModel(state: typeof StateAnnotation.State, config: LangGraphRunnableConfig) {
        const store = config.store;
        const userId = config?.configurable?.user_id;

        if (!store) {
            throw new Error("store is required when compiling the graph");
        }

        if (!userId) {
            throw new Error("userId is required in the config");
        }

        let preferredPlatform: string | undefined;
        try {
            const memories = loadMemory();
            preferredPlatform = memories[userId]?.find((mem: MemoryItem) => mem.key === "platform_preference")?.value?.content;
            console.log("Loaded preferred platform from memory:", preferredPlatform);
        } catch (error) {
            console.error("Error loading memory:", error);
        }

        const configurable = ensureConfiguration(config);
        const lastMessageContent = state.messages[state.messages.length - 1]?.content;
        let messageText: string | undefined;

        if (typeof lastMessageContent === 'string') {
            messageText = lastMessageContent;
        } else if (Array.isArray(lastMessageContent)) {
            const textPart = lastMessageContent.find(item => typeof item === 'string');
            if (typeof textPart === 'string') {
                messageText = textPart;
            }
        }

        const detectedPlatform = messageText ? detectPlatformKeyword(messageText) : undefined;

        if (detectedPlatform) {
            try {
                await upsertMemoryTool.invoke({
                    content: detectedPlatform,
                    context: "User mentioned platform preference.",
                    memoryId: "platform_preference"
                }, config);
                preferredPlatform = detectedPlatform; // Update for the current turn
                console.log("Successfully updated platform preference in memory.");
            } catch (error) {
                console.error("Error updating memory:", error);
            }
        }

        const formatted = preferredPlatform ? `\n<platform_preference>\n${preferredPlatform}\n</platform_preference>` : "";

        const sys = configurable.systemPrompt
            .replace("{user_info}", formatted)
            .replace("{time}", new Date().toISOString());

        const result = await model.invoke([{ role: "system", content: sys }, ...state.messages]);

        return { messages: [result] };
    }

    function routeMessage(state: typeof StateAnnotation.State): "tools" | typeof END {
        const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
        if (lastMessage.tool_calls?.length) {
            return "tools";
        }
        return END;
    }

    const workflow = new StateGraph(StateAnnotation)
        .addNode("agent", callModel)
        .addNode("tools", toolNode)
        .addEdge("__start__", "agent")
        .addConditionalEdges("agent", shouldContinue, {
            tools: "tools",
            end: END,
        })
        .addEdge("tools", "agent");

    const checkpointer = new MemorySaver();
    const inMemoryStore = new InMemoryStore();

    const app = workflow.compile({
        checkpointer,
        store: inMemoryStore,
    });

    const history = (prevMessages ?? [])
        .filter(({ role, content }) => role && content)
        .map(({ role, content }) => (role === "user" ? new HumanMessage(content) : new AIMessage(content)));

    const messages = [
        new SystemMessage(SYSTEM_PROMPT),
        ...history,
        new HumanMessage(humanMessage),
    ];

    const finalState = await app.invoke(
        { messages: messages },
        { configurable: { thread_id: "42", user_id: "42" } }
    );

    return finalState.messages[finalState.messages.length - 1].content;
};

export default chatBot;