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



import { Pool } from 'pg';
require('dotenv').config();

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

async function testConnection() {
  try {
    const client = await pool.connect();
    client.release();
    console.log('Database connected successfully');
  } catch (err) {
    console.error("Error connecting to PostgreSQL", err);
  }
}

export { pool, testConnection };