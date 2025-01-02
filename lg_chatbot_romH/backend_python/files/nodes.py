import uuid
from datetime import datetime
import os
from typing import Literal, Optional, TypedDict, List, Dict, Any
from langchain_core.messages import SystemMessage, HumanMessage, AIMessage
from langchain_core.runnables import RunnableConfig
from langgraph.graph import StateGraph, MessagesState, START, END
from langchain_openai import ChatOpenAI
from trustcall import create_extractor
from langchain_core.messages import merge_message_runs
from utils import extract_tool_info, Spy
from schemas import Profile, ToDo
import configuration

# Load environment variables from .env file
from dotenv import load_dotenv
load_dotenv()

# Initialize the model
model = ChatOpenAI(model="gpt-4o-mini", temperature=0, openai_api_key=os.getenv("OPENAI_API_KEY"))

# Update memory tool
class UpdateMemory(TypedDict):
    """ Decision on what memory type to update """
    update_type: Literal['user', 'todo', 'instructions']

profile_extractor = create_extractor(
    model,
    tools=[Profile],
    tool_choice="Profile",
    enable_inserts=True
)

CREATE_INSTRUCTIONS = """You are a helpful chatbot specializing in video games.

You are designed to be a companion to a user, helping them with information about games and their *currently preferred* gaming platform.

You have a long term memory which keeps track of three things:
1. The user's profile (general information about them, including their *most recently mentioned* gaming platform)
...
Here are your instructions for reasoning about the user's messages:

1. Reason carefully about the user's messages as presented below.

2. Decide whether any of your long-term memory should be updated:
- If the user explicitly mentions a gaming platform (pc, xbox, playstation, ps4, ps5, nintendo switch), *immediately update their profile to reflect this as their current preference.* Call UpdateMemory tool with type `user`.
...
"""

# You might want to adjust TRUSTCALL_INSTRUCTION to emphasize platform extraction
TRUSTCALL_INSTRUCTION = """Reflect on following interaction.

Use the provided tools to retain any necessary memories about the user, especially their preferred gaming platform (if mentioned).

Use parallel tool calling to handle updates and insertions simultaneously.

System Time: {time}"""

def task_mAIstro(state: MessagesState, config: RunnableConfig):
    """Load memories from the store and use them to personalize the chatbot's response."""

    store = config["configurable"]["store"]

    # Get the user ID from the config
    configurable = configuration.Configuration.from_runnable_config(config)
    user_id = configurable.user_id

    # Retrieve profile memory from the store
    namespace = ("profile", user_id)
    memories = store.search(namespace)
    user_profile = memories[0].value if memories else {} # Handle case where profile is empty

    # Retrieve people memory from the store
    namespace = ("todo", user_id)
    memories = store.search(namespace)
    todo = "\n".join(f"{mem.value}" for mem in memories) if memories else ""

    # Retrieve custom instructions
    namespace = ("instructions", user_id)
    memories = store.search(namespace)
    instructions = memories[0].value if memories else ""

    system_msg = CREATE_INSTRUCTIONS.format(user_profile=user_profile, todo=todo, instructions=instructions)

    # Respond using memory as well as the chat history
    # THE KEY FIX IS HERE. I WAS USING PARALLEL TOOL CALLS AND I SHOULDNT HAVE BEEN
    response = model.bind_tools([UpdateMemory], parallel_tool_calls=False).invoke([SystemMessage(content=system_msg)]+state["messages"])

    return {"messages": [response]}

def update_profile(state: MessagesState, config: RunnableConfig):
    """Reflect on the chat history and update the memory collection."""
    # print("INSIDE UPDATE PROFILE..........")

    store = config["configurable"]["store"]

    # Get the user ID from the config
    configurable = configuration.Configuration.from_runnable_config(config)
    user_id = configurable.user_id

    # Define the namespace for the memories
    namespace = ("profile", user_id)

    # Retrieve the most recent memories for context
    existing_items = store.search(namespace)

    # Format the existing memories for the Trustcall extractor
    tool_name = "Profile"
    existing_memories = ([(existing_item.key, tool_name, existing_item.value)
                          for existing_item in existing_items]
                          if existing_items
                          else None
                        )

    # Merge the chat history and the instruction
    TRUSTCALL_INSTRUCTION_FORMATTED=TRUSTCALL_INSTRUCTION.format(time=datetime.now().isoformat())
    updated_messages=list(merge_message_runs(messages=[SystemMessage(content=TRUSTCALL_INSTRUCTION_FORMATTED)] + state["messages"][:-1]))

    # Invoke the extractor
    result = profile_extractor.invoke({"messages": updated_messages,
                                         "existing": existing_memories})

    # Save save the memories from Trustcall to the store
    for r, rmeta in zip(result["responses"], result["response_metadata"]):
        store.put(namespace,
                  rmeta.get("json_doc_id", str(uuid.uuid4())),
                  r.model_dump(mode="json"),
            )
    tool_calls = state['messages'][-1].tool_calls
    # Return tool message with update verification
    return {"messages": [{"role": "tool", "content": "updated profile", "tool_call_id":tool_calls[0]['id']}]}

def update_todos(state: MessagesState, config: RunnableConfig):
    """Reflect on the chat history and update the memory collection."""
    store = config["configurable"]["store"]
    # Get the user ID from the config
    configurable = configuration.Configuration.from_runnable_config(config)
    user_id = configurable.user_id

    # Define the namespace for the memories
    namespace = ("todo", user_id)

    # Retrieve the most recent memories for context
    existing_items = store.search(namespace)

    # Format the existing memories for the Trustcall extractor
    tool_name = "ToDo"
    existing_memories = ([(existing_item.key, tool_name, existing_item.value)
                          for existing_item in existing_items]
                          if existing_items
                          else None
                        )

    # Merge the chat history and the instruction
    TRUSTCALL_INSTRUCTION_FORMATTED=TRUSTCALL_INSTRUCTION.format(time=datetime.now().isoformat())
    updated_messages=list(merge_message_runs(messages=[SystemMessage(content=TRUSTCALL_INSTRUCTION_FORMATTED)] + state["messages"][:-1]))

    # Initialize the spy for visibility into the tool calls made by Trustcall
    spy = Spy()

    # Create the Trustcall extractor for updating the ToDo list
    todo_extractor = create_extractor(
    model,
    tools=[ToDo],
    tool_choice=tool_name,
    enable_inserts=True
    ).with_listeners(on_end=spy)

    # Invoke the extractor
    result = todo_extractor.invoke({"messages": updated_messages,
                                         "existing": existing_memories})

    # Save save the memories from Trustcall to the store
    for r, rmeta in zip(result["responses"], result["response_metadata"]):
        store.put(namespace,
                  rmeta.get("json_doc_id", str(uuid.uuid4())),
                  r.model_dump(mode="json"),
            )

    # Respond to the tool call made in task_mAIstro, confirming the update
    tool_calls = state['messages'][-1].tool_calls

    # Extract the changes made by Trustcall and add the the ToolMessage returned to task_mAIstro
    todo_update_msg = extract_tool_info(spy.called_tools, tool_name)
    return {"messages": [{"role": "tool", "content": todo_update_msg, "tool_call_id":tool_calls[0]['id']}]}

def update_instructions(state: MessagesState, config: RunnableConfig):
    """Reflect on the chat history and update the memory collection."""

    store = config["configurable"]["store"]
    # Get the user ID from the config
    configurable = configuration.Configuration.from_runnable_config(config)
    user_id = configurable.user_id

    namespace = ("instructions", user_id)

    existing_memory = store.get(namespace, "user_instructions")

    # Format the memory in the system prompt
    system_msg = CREATE_INSTRUCTIONS.format(current_instructions=existing_memory.value if existing_memory else None)
    new_memory = model.invoke([SystemMessage(content=system_msg)]+state['messages'][:-1] + [HumanMessage(content="Please update the instructions based on the conversation")])

    # Overwrite the existing memory in the store
    key = "user_instructions"
    store.put(namespace, key, {"memory": new_memory.content})
    tool_calls = state['messages'][-1].tool_calls
    # Return tool message with update verification
    return {"messages": [{"role": "tool", "content": "updated instructions", "tool_call_id":tool_calls[0]['id']}]}

# Conditional edge
def route_message(state: MessagesState, config: RunnableConfig) -> Literal[END, "update_todos", "update_instructions", "update_profile"]:
    """Reflect on the memories and chat history to decide whether to update the memory collection."""
    message = state['messages'][-1]
    # print(f"route_message: Received message with tool calls: {message.tool_calls}") # Added
    if len(message.tool_calls) == 0:
        # print("route_message: No tool calls, returning END") # Added
        return END
    else:
        tool_call = message.tool_calls[0]
        # print(f"route_message: Tool call details: {tool_call}") # Added
        if tool_call['args']['update_type'] == "user":
            # print("route_message: update_type is user, returning update_profile") # Added
            return "update_profile"
        elif tool_call['args']['update_type'] == "todo":
            # print("route_message: update_type is todo, returning update_todos") # Added
            return "update_todos"
        elif tool_call['args']['update_type'] == "instructions":
            # print("route_message: update_type is instructions, returning update_instructions") # Added
            return "update_instructions"
        else:
            raise ValueError