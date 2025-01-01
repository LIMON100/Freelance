# memory_agent.py
import uuid
from datetime import datetime
import os
from typing import Literal, Optional, TypedDict, List, Dict, Any

from flask import Flask, request, jsonify
from flask_cors import CORS
from pydantic import BaseModel, Field
from redis import Redis

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

import configuration

# Load environment variables from .env file
from dotenv import load_dotenv

load_dotenv()


## Utilities 

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
             if call['name'] == 'all_products':
                 changes.append({
                     'type': 'all_products',
                     'query': call['args']['query'],
                 })
             elif call['name'] == 'upsertMemory':
                 changes.append({
                     'type': 'upsertMemory',
                     'content': call['args']['content'],
                     'context': call['args']['context'],
                    'memoryId': call['args'].get('memoryId')
                 })

    # Format results as a single string
    result_parts = []
    for change in changes:
        if change['type'] == 'all_products':
            result_parts.append(f"Searching product: {change['query']}")
        elif change['type'] == 'upsertMemory':
            result_parts.append(
                f"Upserting memory: content={change['content']}, context={change['context']}, memoryId={change.get('memoryId')}"
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

class Product(BaseModel):
    id: int
    name: str
    platform: str
    price: float
    
class ToolCall(BaseModel):
  name: str
  args: Dict[str, Any]

## Initialize the model and tools

# Update memory tool
class UpdateMemory(TypedDict):
    """ Decision on what memory type to update """
    update_type: Literal['user', 'todo', 'instructions']

# Initialize the model
model = ChatOpenAI(model="gpt-4o-mini", temperature=0)

# Initialize embeddings
embeddings = OpenAIEmbeddings(
    api_key=os.getenv("OPENAI_API_KEY"),
    model="text-embedding-3-large",
)

# Database connection parameters from environment variables
DB_USER = os.getenv("DB_USER")
DB_PASSWORD = os.getenv("DB_PASSWORD")
DB_HOST = os.getenv("DB_HOST")
DB_NAME = os.getenv("DB_NAME")
DB_PORT = os.getenv("DB_PORT")
DB_SSL_REJECT_UNAUTHORIZED = os.getenv("DB_SSL_REJECT_UNAUTHORIZED") == "true"

# Create a database pool
def get_db_connection():
    """Get a PostgreSQL database connection."""
    try:
      conn = psycopg2.connect(
        user=DB_USER,
        password=DB_PASSWORD,
        host=DB_HOST,
        database=DB_NAME,
        port=DB_PORT,
        sslmode="require" if DB_SSL_REJECT_UNAUTHORIZED else "disable",
        sslcert="server-ca.pem" if DB_SSL_REJECT_UNAUTHORIZED else None,
    )
      return conn
    except Exception as e:
      print(f"Error connecting to database {e}")
      return None

def fetch_products(query: str, store: Redis) -> str:
  """Fetch products from the database based on the given query."""
  conn = get_db_connection()
  if conn is None:
    return "Error: Could not connect to the database"
    
  try:
      
    search_embedding = embeddings.embed_query(query)

    # Construct the SQL query to perform the vector search
    sql_query = f"""
        SELECT id, name, platform, price
        FROM products
        ORDER BY embedding <=> ARRAY[{','.join(map(str, search_embedding))}]::vector
        LIMIT 20;
    """
    with conn.cursor() as cursor:
      cursor.execute(sql_query)
      rows = cursor.fetchall()
    
    if not rows:
            return "No games found for the given query."
            
    formatted_products = "\n".join(f"{row[1]} (Platform: {row[2]}) (Price: ${row[3]})" for row in rows)
    return formatted_products
  except Exception as e:
        print(f"Error fetching products: {e}")
        return f"Error fetching products: {e}"
  finally:
    conn.close()

# Memory handling using Redis for long-term memory
redis_client = Redis(host='localhost', port=6379, db=0)

# --- Prompts ---

# Chatbot instruction for choosing what to update and what tools to call 
MODEL_SYSTEM_MESSAGE = """You are a helpful chatbot. 

You are designed to be a companion to a user, helping them find games in an online store.

You have a long term memory which keeps track of three things:
1. The user's platform preference 
2. The user's ToDo list
3. General instructions for updating the ToDo list

Here is the current platform preference (may be empty if no information has been collected yet):
<user_info>
{user_info}
</user_info>

Here is the current ToDo List (may be empty if no tasks have been added yet):
<todo>
{todo}
</todo>

Here are the current user-specified preferences for updating the ToDo list (may be empty if no preferences have been specified yet):
<instructions>
{instructions}
</instructions>

System Time: {time}

Here are your instructions for reasoning about the user's messages:

1. Reason carefully about the user's messages as presented below. 

2. Decide whether any of the your long-term memory should be updated:
- If the user mentions a preferred platform (e.g., "I prefer PC" or "I want Xbox games"), call the upsertMemory tool with the input { platform_preference: "platform" } to store this preference.
- If tasks are mentioned, update the ToDo list by calling UpdateMemory tool with type `todo`
- If the user has specified preferences for how to update the ToDo list, update the instructions by calling UpdateMemory tool with type `instructions`

3. Tell the user that you have updated your memory, if appropriate:
- Do not tell the user you have updated the user's profile
- Tell the user them when you update the todo list
- Do not tell the user that you have updated instructions

4. If the user asks for product details use the `all_products` tool.

5. Respond naturally to user user after a tool call was made to save memories, or if no tool call was made."""

# Trustcall instruction
TRUSTCALL_INSTRUCTION = """Reflect on following interaction. 

Use the provided tools to retain any necessary memories about the user. 

Use parallel tool calling to handle updates and insertions simultaneously.

System Time: {time}"""

# Instructions for updating the ToDo list
CREATE_INSTRUCTIONS = """Reflect on the following interaction.

Based on this interaction, update your instructions for how to update ToDo list items. Use any feedback from the user to update how they like to have items added, etc.

Your current instructions are:

<current_instructions>
{current_instructions}
</current_instructions>"""

# --- Node Definitions ---
def should_continue(state: MessagesState) -> Literal["tools",  END]:
    """Determines whether to continue the conversation or to call the tools."""
    last_message = state["messages"][-1]
    if hasattr(last_message, 'tool_calls') and last_message.tool_calls:
        return "tools"
    else:
      return END

def task_mAIstro(state: MessagesState, config: RunnableConfig, store: Redis):
    """Load memories from the store and use them to personalize the chatbot's response."""
    
    # Get the user ID from the config
    configurable = configuration.Configuration.from_runnable_config(config)
    user_id = configurable.user_id

    # Retrieve platform preference from Redis
    user_info_key = f"user_platform_preference:{user_id}"
    user_platform_preference = store.get(user_info_key)
    if user_platform_preference:
        user_platform_preference =  f"<platform_preference>{user_platform_preference.decode('utf-8')}</platform_preference>"
    else:
        user_platform_preference = ""
        

    # Retrieve todo memory from the store
    todo_key = f"todo:{user_id}"
    todo_items = store.get(todo_key)
    if todo_items:
        todo = todo_items.decode("utf-8")
    else:
        todo = ""


    # Retrieve custom instructions
    instructions_key = f"instructions:{user_id}"
    instructions_mem = store.get(instructions_key)
    if instructions_mem:
        instructions = instructions_mem.decode("utf-8")
    else:
        instructions = ""
    
    system_msg = MODEL_SYSTEM_MESSAGE.format(user_info=user_platform_preference, todo=todo, instructions=instructions, time=datetime.now().isoformat())

    # Respond using memory as well as the chat history
    response = model.bind_tools([ToolCall], parallel_tool_calls=True).invoke([SystemMessage(content=system_msg)]+state["messages"])
    return {"messages": [response]}

def call_tools(state: MessagesState, config: RunnableConfig, store: Redis):

    """Reflect on the chat history and update the memory collection."""
    
    # Get the user ID from the config
    configurable = configuration.Configuration.from_runnable_config(config)
    user_id = configurable.user_id
    
    # Initialize the spy for visibility into the tool calls made by the LLM
    spy = Spy()

    # Get the latest response from the LLM
    messages = state["messages"]
    latest_message = messages[-1]
    
    # Get the tool calls
    tool_calls = latest_message.tool_calls

    if not tool_calls:
      return {"messages": []}
      
    # Process each tool call
    tool_results = []
    for call in tool_calls:
        if call.name == "all_products":
            tool_results.append(fetch_products(call.args["query"], store))
        elif call.name == "upsertMemory":
            # Save to Redis
            user_info_key = f"user_platform_preference:{user_id}"
            store.set(user_info_key, call.args["content"])
            tool_results.append("Memory updated.")
        else:
            tool_results.append(f"Tool {call.name} is not recognized")
    
    # Respond to the tool call made in task_mAIstro, confirming the update   
    tool_update_msg = "\n".join(tool_results)
    
    return {"messages": [AIMessage(content=tool_update_msg, tool_calls=tool_calls)]}



# Conditional edge
def route_message(state: MessagesState, config: RunnableConfig, store: Redis) -> Literal[END, "tools"]:
    """Reflect on the memories and chat history to decide whether to update the memory collection."""
    message = state['messages'][-1]
    if hasattr(message, 'tool_calls') and message.tool_calls:
      return "tools"
    else:
      return END

# Create the graph + all nodes
builder = StateGraph(MessagesState, config_schema=configuration.Configuration)

# Define the flow of the memory extraction process
builder.add_node("agent", task_mAIstro)
builder.add_node("tools", call_tools)
# builder.add_node(END) # we add END node here

# Define the flow 
builder.add_edge(START, "agent")
builder.add_conditional_edges("agent", should_continue)
builder.add_edge("tools", "agent")
# builder.add_edge("agent", END) # add direct edge to the end if conditional not met
# Compile the graph
graph = builder.compile()


# --- Flask App ---
app = Flask(__name__)
CORS(app, resources={r"/chat": {"origins": "http://localhost:3001"}})

@app.route("/chat", methods=["POST"])
def chat_endpoint():
    """Endpoint for handling chat requests from the frontend."""
    data = request.get_json()
    user_message = data.get("message")
    prev_messages = data.get("prevMessages", [])
    user_id = data.get("userId", "default_user")  # Add a user ID parameter

    if not user_message:
        return jsonify({"error": "No message provided"}), 400

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
        config={"configurable": {"user_id": user_id}},
    )
    last_message = final_state.get("messages", [])[-1] if final_state.get("messages") else AIMessage(content="Sorry! Something went wrong")

    return jsonify({"response": last_message.content})

if __name__ == "__main__":
    app.run(debug=True, port=5001)