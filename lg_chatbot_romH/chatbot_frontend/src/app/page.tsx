"use client"
import DialogflowWidget from '@/app/ui/DialogflowWidget';

import { useState } from 'react';
import AIChatButton from "@/app/components/AIChatButton";

export default function Chat() {
  const [messages, setMessages] = useState([]);
  const [input, setInput] = useState('');

  const handleSendMessage = async () => {
    const userMessage = { sender: 'user', text: input };
    setMessages([...messages, userMessage]);

    const response = await fetch('http://localhost:5002/api/chat', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ message: input, prevMessages: messages })
    });

    const botMessage = await response.json();
    setMessages([...messages, userMessage, botMessage]);
    setInput('');
  };

  return (
    <div>
      {/*<h1>Chatbot</h1>*/}
      {/*<div>*/}
      {/*  {messages.map((msg, index) => (*/}
      {/*    <div key={index} className={msg.sender}>*/}
      {/*      <p>{msg.text}</p>*/}
      {/*    </div>*/}
      {/*  ))}*/}
      {/*</div>*/}
      {/*<input*/}
      {/*  type="text"*/}
      {/*  value={input}*/}
      {/*  onChange={(e) => setInput(e.target.value)}*/}
      {/*/>*/}
      {/*<button onClick={handleSendMessage}>Send</button>*/}

      {/*<DialogflowWidget/>*/}

      <AIChatButton />
    </div>
  );
}
