import { AIMessage, BaseMessage, HumanMessage, SystemMessage } from "@langchain/core/messages";
import { LangGraphRunnableConfig, MemorySaver, StateGraph, END } from "@langchain/langgraph";
import { ToolNode } from "@langchain/langgraph/prebuilt";
import { ChatOpenAI } from "@langchain/openai";
import upsertMemoryTool from "./tools/upsertMemoryTool";
import { SYSTEM_PROMPT } from "./prompts/prompts";
import { ConfigurationAnnotation, ensureConfiguration } from "./configuration/configuration";
import allProductsTool from "./tools/allProductsTool";
import { DynamicStructuredTool,  Tool } from "@langchain/core/tools";
import { pool } from "./db/db"; // Import your database pool
import { InMemoryStore } from "@langchain/langgraph";


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

         const configurable = ensureConfiguration(config);
        let formatted = "";

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

            formatted = `\n<platform_preference>\n${detectedPlatform}\n</platform_preference>`;

           console.log("Updated the platform to:", detectedPlatform);
        } catch (error) {
            console.error("Failed to update platform preference", error);
        }
    }

        const sys = configurable.systemPrompt
            .replace("{user_info}", formatted)
            .replace("{time}", new Date().toISOString());

         const result = await model.invoke([{ role: "system", content: sys }, ...state.messages]);

        return { messages: [result] };
    }

    async function storeMemory(state: typeof StateAnnotation.State, config: LangGraphRunnableConfig): Promise<{ messages: BaseMessage[] }> {
        const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
        const toolCalls = lastMessage.tool_calls || [];
          console.log("toolCalls in storeMemory:", toolCalls)
        let savedMemories: BaseMessage[] = [];
        if (toolCalls.length > 0) {
            savedMemories = await Promise.all(
                toolCalls.map(async (tc) => {
                    const tool = tools.find((tool) => tool.name === tc.name);
                     if (!tool) {
                       return null
                     }
                    try {
                       return await tool.invoke(tc.args, config)
                    } catch (error) {
                        console.error(`Error invoking tool ${tool.name}:`, error);
                        return null
                    }
                }),
            );
        }
          return { messages: savedMemories.filter(Boolean) } as { messages: BaseMessage[] };
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
          .addNode("store_memory", storeMemory)
        .addEdge("__start__", "agent")
        .addConditionalEdges("agent", shouldContinue, {
            tools: "tools",
            end: END,
        })
         .addEdge("tools", "store_memory")
        .addEdge("store_memory", "agent");


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