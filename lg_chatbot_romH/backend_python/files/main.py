# main.py
import uuid
from datetime import datetime
import os
from typing import Literal, Optional, TypedDict, List, Dict, Any
from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
from langchain_core.messages import SystemMessage, HumanMessage, AIMessage
from langgraph.graph import StateGraph, MessagesState, START, END

import configuration
from utils import PostgresStore
from nodes import task_mAIstro, update_profile, update_todos, update_instructions, route_message
from schemas import  _MemoryItem

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

    app.run(debug=True, port=5002)