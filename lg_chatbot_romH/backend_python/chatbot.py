from langchain_core.messages import AIMessage, BaseMessage, HumanMessage, SystemMessage
from langchain_openai import ChatOpenAI
from langgraph.checkpoint.memory import MemorySaver
from langgraph.graph import StateGraph, MessagesState, START, END
from langgraph.store.memory import InMemoryStore
from langgraph.configuration import Configuration as LangGraphConfiguration
import configuration
import upsertMemoryTool
import allProductsTool

async def chatBot(humanMessage, prevMessages):
    StateAnnotation = configuration.ConfigurationAnnotation

    tools = [
        allProductsTool.default,
        upsertMemoryTool.default,
    ]
    toolNode = ToolNode(tools)

    model = ChatOpenAI(
        model="gpt-4o-mini",
        temperature=0,
        cache=True,
    ).bindTools(tools)

    def shouldContinue(state):
        messages = state.messages
        lastMessage = messages[messages.length - 1]
        if lastMessage.tool_calls:
            return "tools"
        return "end"

    async def callModel(state, config):
        store = config.store
        userId = config.configurable.user_id
        if not store:
            raise ValueError("store is required when compiling the graph")
        if not userId:
            raise ValueError("userId is required in the config")

        memories = await store.search(["12", "memories"])
        formatted = "\n".join([f"{mem.key}: {mem.value}" for mem in memories]) if memories else ""

        configurable = configuration.ensureConfiguration(config)
        sys = configurable.systemPrompt.replace("{user_info}", formatted).replace("{time}", new Date().toISOString())

        result = await model.invoke([{ "role": "system", "content": sys }, *state.messages])
        return { "messages": [result] }

    async def storeMemory(state):
        lastMessage = state.messages[state.messages.length - 1]
        toolCalls = lastMessage.tool_calls or []
        savedMemories = await Promise.all([upsertMemoryTool.default.invoke(tc) for tc in toolCalls])
        return { "messages": savedMemories }

    def routeMessage(state):
        lastMessage = state.messages[state.messages.length - 1]
        if lastMessage.tool_calls:
            return "store_memory"
        return END

    workflow = StateGraph(StateAnnotation)
    workflow.add_node("agent", callModel)
    workflow.add_node("tools", toolNode)
    workflow.add_edge("start", "agent")
    workflow.add_conditional_edges("agent", shouldContinue)
    workflow.add_edge("tools", "agent")

    checkpointer = MemorySaver()
    inMemoryStore = InMemoryStore()

    app = workflow.compile({
        "checkpointer": checkpointer,
        "store": inMemoryStore,
    })

    history = [HumanMessage(content) if role == 'user' else AIMessage(content) for role, content in (prevMessages or [])]
    messages = [SystemMessage(configuration.SYSTEM_PROMPT), *history, HumanMessage(humanMessage)]

    finalState = await app.invoke({ "messages": messages }, { "configurable": { "thread_id": "42", "user_id": "42" } })
    return finalState.messages[finalState.messages.length - 1].content