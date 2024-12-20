// import { Pool } from 'pg';

// require('dotenv').config();

// const pool = new Pool({
//   user: process.env.DB_USER,
//   host: process.env.DB_HOST,
//   database: process.env.DB_NAME,
//   password: process.env.DB_PASSWORD,
//   port: Number(process.env.DB_PORT),
//   ssl: {
//     rejectUnauthorized: process.env.DB_SSL_REJECT_UNAUTHORIZED === 'true',
//   },
// });

// async function testConnection() {
//   try {
//     const client = await pool.connect();
//     client.release();
//     console.log('Database connected successfully');
//   } catch (err) {
//     console.error("Error connecting to PostgreSQL", err);
//   }
// }

// export { pool, testConnection };



// db.ts
import { Pool, QueryResult } from 'pg';
import 'dotenv/config';

const pool = new Pool({
    user: process.env.DB_USER,
    host: process.env.DB_HOST,
    database: process.env.DB_NAME,
    password: process.env.DB_PASSWORD,
    port: Number(process.env.DB_PORT),
    ssl: {
        rejectUnauthorized: process.env.DB_SSL_REJECT_UNAUTHORIZED === 'true',
    },
});

export const testConnection = async () => {
    try {
        const client = await pool.connect();
        client.release();
        console.log('Database connected successfully');
    } catch (err) {
        console.error("Error connecting to PostgreSQL", err);
    }
};

// New function to insert or update memories
export const upsertMemoryInDb = async (userId: string, memoryId: string, content: string, context: string): Promise<QueryResult<any>> => {
    const query = `
        INSERT INTO memories (user_id, memory_id, content, context) 
        VALUES ($1, $2, $3, $4)
        ON CONFLICT (user_id, memory_id) DO UPDATE 
        SET content = $3, context = $4;
    `;
    return pool.query(query, [userId, memoryId, content, context]);
};
// New function to retrieve memories
export const getMemoriesFromDb = async (userId: string): Promise<QueryResult<any>> => {
    const query = `
        SELECT memory_id, content, context FROM memories WHERE user_id = $1;
    `;
    return pool.query(query, [userId]);
};


export default pool;