import express, { Request, Response } from 'express';
import cors from 'cors';
import bodyParser from 'body-parser';
import {testConnection} from './db/db';
import chatBot from "./chatBot";

require('dotenv').config();

const app = express();

app.use(cors({
  origin: 'http://localhost:3000',
}));

app.use(bodyParser.json());

const PORT = 5002;

export type PrevMessages = {
  id: string;
  role: "user" | "assistant";
  content: string;
}

app.post('/api/chat', async (req: Request, res: Response) => {
  const userMessage: string = req.body.message;
  const prevMessages: PrevMessages[] = req.body.prevMessages;

  const resultAIChatbot = await chatBot(userMessage, prevMessages);

  const botResponse = { role: 'assistant', content: resultAIChatbot };
  res.json(botResponse);
});

app.listen(PORT, () => {
  console.log(`Backend server running on http://localhost:${PORT}`);
});

testConnection();
