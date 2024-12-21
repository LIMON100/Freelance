CREATE TABLE memories (
    id UUID PRIMARY KEY,
    user_id VARCHAR(255) NOT NULL,
    content TEXT NOT NULL,
    context TEXT
);