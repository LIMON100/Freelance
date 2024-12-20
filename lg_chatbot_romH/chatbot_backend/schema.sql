CREATE TABLE IF NOT EXISTS user_memories (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(255) NOT NULL,
    memory_id VARCHAR(255) NOT NULL UNIQUE,
    content TEXT,
    context TEXT
);