import uuid
from datetime import datetime
import os
from typing import Literal, Optional, TypedDict, List, Dict, Any

from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
from pydantic import BaseModel, Field

from langchain_core.runnables import RunnableConfig
from langchain_core.messages import merge_message_runs
from langchain_core.messages import SystemMessage, HumanMessage, AIMessage

from langchain_openai import ChatOpenAI
from langchain_openai import OpenAIEmbeddings

from langgraph.checkpoint.memory import MemorySaver
from langgraph.graph import StateGraph, MessagesState, START, END
from langgraph.store.base import BaseStore
from langgraph.store.memory import InMemoryStore
import psycopg2
from psycopg2.extras import RealDictCursor
from trustcall import create_extractor

import configuration

# Load environment variables from .env file
from dotenv import load_dotenv

load_dotenv()


# User profile schema
class Profile(BaseModel):
    """This is the profile of the user you are chatting with"""
    name: Optional[str] = Field(description="The user's name", default=None)
    location: Optional[str] = Field(description="The user's location", default=None)
    job: Optional[str] = Field(description="The user's job", default=None)
    connections: list[str] = Field(
        description="Personal connection of the user, such as family members, friends, or coworkers",
        default_factory=list
    )
    interests: list[str] = Field(
        description="Interests that the user has",
        default_factory=list
    )
    platform_preference: Optional[Literal['pc', 'xbox', 'playstation', 'ps4', 'ps5', 'nintendo switch']] = Field(
        description="The user's preferred gaming platform", default=None
    )



# Inspect the tool calls for Trustcall
class Spy:
    def __init__(self):
        self.called_tools = []

    def __call__(self, run):
        q = [run]
        while q:
            r = q.pop()
            if r.child_runs:
                q.extend(r.child_runs)
            if r.run_type == "chat_model":
                self.called_tools.append(
                    r.outputs["generations"][0][0]["message"]["kwargs"]["tool_calls"]
                )

spy = Spy()
# Extract information from tool calls for both patches and new memories in Trustcall
def extract_tool_info(tool_calls, schema_name="Memory"):
    """Extract information from tool calls for both patches and new memories.

    Args:
        tool_calls: List of tool calls from the model
        schema_name: Name of the schema tool (e.g., "Memory", "ToDo", "Profile")
    """

    # Initialize list of changes
    changes = []

    for call_group in tool_calls:
        for call in call_group:
            if call['name'] == 'PatchDoc':
                changes.append({
                    'type': 'update',
                    'doc_id': call['args']['json_doc_id'],
                    'planned_edits': call['args']['planned_edits'],
                    'value': call['args']['patches'][0]['value']
                })
            elif call['name'] == schema_name:
                changes.append({
                    'type': 'new',
                    'value': call['args']
                })

    # Format results as a single string
    result_parts = []
    for change in changes:
        if change['type'] == 'update':
            result_parts.append(
                f"Document {change['doc_id']} updated:\n"
                f"Plan: {change['planned_edits']}\n"
                f"Added content: {change['value']}"
            )
        else:
            result_parts.append(
                f"New {schema_name} created:\n"
                f"Content: {change['value']}"
            )

    return "\n\n".join(result_parts)

## Schema definitions

# User profile schema
class Profile(BaseModel):
    """This is the profile of the user you are chatting with"""
    name: Optional[str] = Field(description="The user's name", default=None)
    location: Optional[str] = Field(description="The user's location", default=None)
    job: Optional[str] = Field(description="The user's job", default=None)
    connections: list[str] = Field(
        description="Personal connection of the user, such as family members, friends, or coworkers",
        default_factory=list
    )
    interests: list[str] = Field(
        description="Interests that the user has",
        default_factory=list
    )

# ToDo schema
class ToDo(BaseModel):
    task: str = Field(description="The task to be completed.")
    time_to_complete: Optional[int] = Field(description="Estimated time to complete the task (minutes).")
    deadline: Optional[datetime] = Field(
        description="When the task needs to be completed by (if applicable)",
        default=None
    )
    solutions: list[str] = Field(
        description="List of specific, actionable solutions (e.g., specific ideas, service providers, or concrete options relevant to completing the task)",
        min_items=1,
        default_factory=list
    )
    status: Literal["not started", "in progress", "done", "archived"] = Field(
        description="Current status of the task",
        default="not started"
    )

## Initialize the model and tools

# Update memory tool
class UpdateMemory(TypedDict):
    """ Decision on what memory type to update """
    update_type: Literal['user', 'todo', 'instructions']

# Initialize the model
model = ChatOpenAI(model="gpt-4o-mini", temperature=0)

## Create the Trustcall extractors for updating the user profile and ToDo list
profile_extractor = create_extractor(
    model,
    tools=[Profile],
    tool_choice="Profile",
)

## Prompts

# Chatbot instruction for choosing what to update and what tools to call
# CREATE_INSTRUCTIONS = """You are a helpful chatbot specializing in video games.

# You are designed to be a companion to a user, helping them with information about games and their *currently discussed* gaming platform.

# You have a long term memory which keeps track of three things:
# 1. The user's profile (general information about them, including their preferred gaming platform)
# 2. The user's ToDo list
# 3. General instructions for updating the ToDo list

# Here is the current User Profile (may be empty if no information has been collected yet):
# <user_profile>
# {user_profile}
# </user_profile>

# Here is the current ToDo List (may be empty if no tasks have been added yet):
# <todo>
# {todo}
# </todo>

# Here are the current user-specified preferences for updating the ToDo list (may be empty if no preferences have been specified yet):
# <instructions>
# {instructions}
# </instructions>

# Here are your instructions for reasoning about the user's messages:

# 1. Reason carefully about the user's messages as presented below.

# 2. Decide whether any of the your long-term memory should be updated:
# - If personal information was provided about the user, *including their preferred gaming platform (NinTech, PlayStation, PC, or Xbox)*, update the user's profile by calling UpdateMemory tool with type `user`
# - If tasks are mentioned, update the ToDo list by calling UpdateMemory tool with type `todo`
# - If the user has specified preferences for how to update the ToDo list, update the instructions by calling UpdateMemory tool with type `instructions`

# 3. Tell the user that you have updated your memory, if appropriate:
# - Do not tell the user you have updated the user's profile (including platform preference)
# - Tell the user them when you update the todo list
# - Do not tell the user that you have updated instructions

# 4. Err on the side of updating the todo list. No need to ask for explicit permission.

# 5. Respond naturally to user user after a tool call was made to save memories, or if no tool call was made.
# """

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

class PostgresStore:
    """
    A PostgreSQL-based store for LangGraph memory.
    """
    def __init__(self):
        self.conn = psycopg2.connect(
            user=os.getenv("DB_USER"),
            password=os.getenv("DB_PASSWORD"),
            host=os.getenv("DB_HOST"),
            port=os.getenv("DB_PORT"),
            database=os.getenv("DB_NAME")
        )
        self.cursor = self.conn.cursor(cursor_factory=RealDictCursor)
        self._initialize_tables()

    def _initialize_tables(self):
        self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS memory_items (
                namespace VARCHAR(255),
                key VARCHAR(255),
                value JSONB,
                PRIMARY KEY (namespace, key)
            );
        """)
        self.conn.commit()

    def put(self, namespace: tuple, key: str, value: dict):
        """Store a value in PostgreSQL."""
        try:
            self.cursor.execute(
                "INSERT INTO memory_items (namespace, key, value) VALUES (%s, %s, %s) ON CONFLICT (namespace, key) DO UPDATE SET value = %s",
                (self._format_namespace(namespace), key, psycopg2.extras.Json(value), psycopg2.extras.Json(value))
            )
            self.conn.commit()
        except Exception as e:
            self.conn.rollback()
            print(f"Error putting data into PostgreSQL: {e}")
            raise

    def get(self, namespace: tuple, key: str):
        """Get a single item from the store."""
        self.cursor.execute(
            "SELECT value FROM memory_items WHERE namespace = %s AND key = %s",
            (self._format_namespace(namespace), key)
        )
        result = self.cursor.fetchone()
        if result:
            return _MemoryItem(key=key, value=result['value'])
        return None

    def search(self, namespace: tuple) -> List["_MemoryItem"]:
        """Search for items by namespace."""
        self.cursor.execute(
            "SELECT key, value FROM memory_items WHERE namespace = %s",
            (self._format_namespace(namespace),)
        )
        items = self.cursor.fetchall()
        return [ _MemoryItem(key=item['key'], value=item['value']) for item in items]

    def _format_namespace(self, namespace: tuple) -> str:
        return ":".join(namespace)

class _MemoryItem(BaseModel):
    key: str
    value: dict

## Node definitions


def task_mAIstro(state: MessagesState, config: RunnableConfig):
    """Load memories from the store and use them to personalize the chatbot's response."""

    store = config["configurable"]["store"]

    # Get the user ID from the config
    configurable = configuration.Configuration.from_runnable_config(config)
    user_id = configurable.user_id

    # Retrieve profile memory from the store
    namespace = ("profile", user_id)
    memories = store.search(namespace)
    user_profile = memories[0].value if memories else {}

    # Retrieve people memory from the store
    namespace = ("todo", user_id)
    memories = store.search(namespace)
    todo = "\n".join(f"{mem.value}" for mem in memories)

    # Retrieve custom instructions
    namespace = ("instructions", user_id)
    memories = store.search(namespace)
    instructions = memories[0].value if memories else ""

    system_msg = CREATE_INSTRUCTIONS.format(user_profile=user_profile, todo=todo, instructions=instructions)

    # Respond using memory as well as the chat history
    # Bind the UpdateMemory tool here!
    response = model.bind_tools([UpdateMemory], parallel_tool_calls=False).invoke([SystemMessage(content=system_msg)] + state["messages"])

    return {"messages": [response]}


def update_profile(state: MessagesState, config: RunnableConfig):
    """Reflect on the chat history and update the memory collection."""
    print("INSIDE UPDATE PROFILE..........")

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
    result = profile_extractor.invoke({"messages": updated_messages, "existing": existing_memories})

    # Save save the memories from Trustcall to the store
    for r, rmeta in zip(result["responses"], result["response_metadata"]):
        store.put(namespace,
                  rmeta.get("json_doc_id", str(uuid.uuid4())),
                  r.model_dump(mode="json"),
            )
    tool_calls = state['messages'][-1].tool_calls
    # Return tool message with update verification
    return {"messages": [{"role": "tool", "content": "updated profile", "tool_call_id":tool_calls[0]['id']}]}

# def update_todos(state: MessagesState, config: RunnableConfig):
#     """Reflect on the chat history and update the memory collection."""
#     store = config["configurable"]["store"]
#     # Get the user ID from the config
#     configurable = configuration.Configuration.from_runnable_config(config)
#     user_id = configurable.user_id

#     # Define the namespace for the memories
#     namespace = ("todo", user_id)

#     # Retrieve the most recent memories for context
#     existing_items = store.search(namespace)

#     # Format the existing memories for the Trustcall extractor
#     tool_name = "ToDo"
#     existing_memories = ([(existing_item.key, tool_name, existing_item.value)
#                           for existing_item in existing_items]
#                           if existing_items
#                           else None
#                         )

#     # Merge the chat history and the instruction
#     TRUSTCALL_INSTRUCTION_FORMATTED=TRUSTCALL_INSTRUCTION.format(time=datetime.now().isoformat())
#     updated_messages=list(merge_message_runs(messages=[SystemMessage(content=TRUSTCALL_INSTRUCTION_FORMATTED)] + state["messages"][:-1]))

#     # Initialize the spy for visibility into the tool calls made by Trustcall
#     spy = Spy()

#     # Create the Trustcall extractor for updating the ToDo list
#     todo_extractor = create_extractor(
#     model,
#     # tools=[ToDo],
#     # tool_choice=tool_name,
#     tools=[Profile],  # Ensure your updated Profile schema is used here
#     tool_choice="Profile",
#     enable_inserts=True
#     ).with_listeners(on_end=spy)

#     # Invoke the extractor
#     result = todo_extractor.invoke({"messages": updated_messages,
#                                          "existing": existing_memories})

#     # Save save the memories from Trustcall to the store
#     for r, rmeta in zip(result["responses"], result["response_metadata"]):
#         store.put(namespace,
#                   rmeta.get("json_doc_id", str(uuid.uuid4())),
#                   r.model_dump(mode="json"),
#             )

#     # Respond to the tool call made in task_mAIstro, confirming the update
#     tool_calls = state['messages'][-1].tool_calls

#     # Extract the changes made by Trustcall and add the the ToolMessage returned to task_mAIstro
#     todo_update_msg = extract_tool_info(spy.called_tools, tool_name)
#     return {"messages": [{"role": "tool", "content": todo_update_msg, "tool_call_id":tool_calls[0]['id']}]}

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
    tools=[ToDo],  # Corrected: Use the ToDo schema
    tool_choice=tool_name, # Corrected: Use the ToDo tool name
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

# Create the graph + all nodes
builder = StateGraph(MessagesState, config_schema=configuration.Configuration)

# Define the flow of the memory extraction process
builder.add_node(task_mAIstro)
builder.add_node(update_todos)
builder.add_node(update_profile)
builder.add_node(update_instructions)

# Define the flow
builder.add_edge(START, "task_mAIstro")
builder.add_conditional_edges("task_mAIstro", route_message)
builder.add_edge("update_todos", "task_mAIstro")
builder.add_edge("update_profile", "task_mAIstro")
builder.add_edge("update_instructions", "task_mAIstro")

# Compile the graph
graph = builder.compile()

# --- Flask App ---
app = Flask(__name__)
CORS(app, resources={r"/*": {"origins": "*"}})

# Initialize a chat history to maintain state between runs
chat_history = {}
postgres_store = PostgresStore()

@app.route("/")
def home():
    return render_template("chat.html")

@app.route("/chat", methods=["POST"])
def chat_endpoint():
    """Endpoint for handling chat requests from the frontend."""
    data = request.get_json()
    user_message = data.get("message")
    user_id = data.get("userId", "default_user") # Add a user ID parameter

    if not user_message:
        return jsonify({"error": "No message provided"}), 400

    # Fetch Previous message from chat history
    prev_messages = chat_history.get(user_id, [])

    messages = [
        SystemMessage(content="You are a helpful game shop assistant."),
    ]

    # Handle previous messages
    for msg in prev_messages:
        if msg["role"] == "user":
            messages.append(HumanMessage(content=msg["content"]))
        elif msg["role"] == "assistant":
            messages.append(AIMessage(content=msg["content"]))

    messages.append(HumanMessage(content=user_message))

    final_state = graph.invoke(
        {"messages": messages},
        config={"configurable": {"user_id": user_id, "store": postgres_store}}
    )
    last_message = final_state.get("messages", [])[-1] if final_state.get("messages") else AIMessage(content="Sorry! Something went wrong")

    # Update the chat history
    prev_messages.append({"role": "user", "content": user_message})
    prev_messages.append({"role": "assistant", "content": last_message.content})
    chat_history[user_id] = prev_messages

    return jsonify({"response": last_message.content})

if __name__ == "__main__":
    app.run(debug=True, port=5002)
