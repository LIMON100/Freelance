# utils.py
import os
import psycopg2
from psycopg2.extras import RealDictCursor
from typing import List
from pydantic import BaseModel
import json
# Load environment variables from .env file
from dotenv import load_dotenv
load_dotenv()
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
# Database connection parameters from environment variables
DB_USER = os.getenv("DB_USER")
DB_PASSWORD = os.getenv("DB_PASSWORD")
DB_HOST = os.getenv("DB_HOST")
DB_NAME = os.getenv("DB_NAME")
DB_PORT = os.getenv("DB_PORT")

def get_db_connection():
    """Get a PostgreSQL database connection."""
    try:
        conn = psycopg2.connect(
            user=os.getenv("DB_USER"),
            password=os.getenv("DB_PASSWORD"),
            host=os.getenv("DB_HOST"),
            database=os.getenv("DB_NAME"),
            port=os.getenv("DB_PORT")
        )
        return conn
    except Exception as e:
        print(f"Error connecting to database {e}")
        return None

class PostgresStore:
    """
    A PostgreSQL-based store for LangGraph memory.
    """
    def __init__(self):
        self.conn = get_db_connection()
        if self.conn is None:
            raise Exception("Could not connect to the database")
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