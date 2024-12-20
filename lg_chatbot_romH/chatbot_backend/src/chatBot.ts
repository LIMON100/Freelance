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

import {AIMessage, BaseMessage, HumanMessage, SystemMessage} from "@langchain/core/messages";
import { LangGraphRunnableConfig, MemorySaver, StateGraph, END } from "@langchain/langgraph";
import {ToolNode} from "@langchain/langgraph/prebuilt";
import {ChatOpenAI} from "@langchain/openai";
import { InMemoryStore } from "@langchain/langgraph";
import upsertMemoryTool from "./tools/upsertMemoryTool";
import { SYSTEM_PROMPT } from "./prompts/prompts";
import { ConfigurationAnnotation, ensureConfiguration } from "./configuration/configuration";
import priceByProductTool from "./tools/priceByProductTool";
import allProductsTool from "./tools/allProductsTool";
import { getMemoriesFromDb } from "./db/db";

export type PrevMessages = {
    id: string;
    role: "user" | "assistant";
    content: string;
}

const chatBot = async (humanMessage: string, prevMessages: PrevMessages[]) => {
    const StateAnnotation = ConfigurationAnnotation;

    // Define the tools for the agent to use
    const tools = [
        allProductsTool,
        upsertMemoryTool,
        // priceByProductTool
    ];
    const toolNode = new ToolNode(tools);

    const model = new ChatOpenAI({
        model: "gpt-4o-mini",
        temperature: 0,
        cache: true,
    }).bindTools(tools);


    // Define the function that determines whether to continue or not
    // We can extract the state typing via StateAnnotation.State
    function shouldContinue(state: typeof StateAnnotation.State) {
        const messages = state.messages;
        const lastMessage = messages[messages.length - 1] as AIMessage;

        // If the LLM makes a tool call, then we route to the "tools" node
        if (lastMessage.tool_calls?.length) {
            return "tools";
        }
        // Otherwise, we stop (reply to the user)
        return "end";
    }

    // Define the function that calls the model
    async function callModel(
        state: typeof StateAnnotation.State,
        config: LangGraphRunnableConfig
    ) {
        const userId = config.configurable?.user_id;

        if (!userId) {
            throw new Error("userId is required in the config");
        }

        const memories = await getMemoriesFromDb(userId)
        // TODO: 1. Check the console.log bellow.
        //  You can easily invoke a upsertMemory tool by message in a chat "my platform is pc".
        //  After the second iteration, the memories are gone.
        //  Do they need to be stored in a database?

        console.log("memories callModel", memories);

        let formatted =
            memories.rows
                ?.map((mem) => `[${mem.memory_id}]: ${JSON.stringify(mem)}`)
                ?.join("\n") || "";

        if (formatted) {
            formatted = `\n<memories>\n${formatted}\n</memories>`;
        }

        const configurable = ensureConfiguration(config);

        const sys = SYSTEM_PROMPT()
            .replace("{user_info}", formatted)
            .replace("{time}", new Date().toISOString());

        // const messages = state.messages;
        //
        // const response = await model.invoke(messages);
        //
        // // We return a list, because this will get added to the existing list
        // return { messages: [response] };

        const result = await model.invoke(
            [{ role: "system", content: sys }, ...state.messages],
        );

        return { messages: [result] };
    }

    async function storeMemory(
        state: typeof StateAnnotation.State,
    ): Promise<{ messages: BaseMessage[] }> {
        const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
        const toolCalls = lastMessage.tool_calls || [];
        const savedMemories = await Promise.all(
            toolCalls.map(async (tc) => {
                return await upsertMemoryTool.invoke(tc);
            }),
        );
    
        console.log("savedMemories:", savedMemories);
        return { messages: state.messages };
    }

    function routeMessage(
        state: typeof StateAnnotation.State,
    ): "store_memory" | typeof END {
        const lastMessage = state.messages[state.messages.length - 1] as AIMessage;
        if (lastMessage.tool_calls?.length) {
            return "store_memory";
        }
        return END;
    }

    // Define a new graph
    const workflow = new StateGraph(StateAnnotation)
        .addNode("agent", callModel)
        .addNode("store_memory", storeMemory)
        .addNode("tools", toolNode)
        .addEdge("__start__", "agent")
        .addConditionalEdges("agent", routeMessage, {
        store_memory: "store_memory",
        [END]: END,
        })
        .addEdge("store_memory", "agent")
        .addConditionalEdges("agent", shouldContinue)
        .addEdge("tools", "agent");

    // Initialize memory to persist state between graph runs
    const checkpointer = new MemorySaver();

    const inMemoryStore = new InMemoryStore();


    // Finally, we compile it!
    // This compiles it into a LangChain Runnable.
    // Note that we're (optionally) passing the memory when compiling the graph
    const app = workflow.compile({
        checkpointer,
        store: inMemoryStore,
    });

    const history = (prevMessages ?? [])
        .filter(({ role, content }) => role && content)
        .map(({ role, content }) =>
            role === 'user' ? new HumanMessage(content) : new AIMessage(content)
        );

    const messages = [
        new SystemMessage(
            SYSTEM_PROMPT()
        ),
        ...history,
        new HumanMessage(humanMessage),
    ];

    // Use the Runnable
    const finalState = await app.invoke(
        { messages: messages },
        { configurable: { thread_id: "42", user_id: "42" } },
    );

    return finalState.messages[finalState.messages.length - 1].content;
}

export default chatBot;