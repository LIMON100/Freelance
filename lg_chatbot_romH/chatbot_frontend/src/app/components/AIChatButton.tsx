"use client";
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { useState } from "react";
import AIChatBox from "./AIChatBox";
import { faRocket } from '@fortawesome/free-solid-svg-icons'

export default function AIChatButton() {
  const [chatBoxOpen, setChatBoxOpen] = useState(false);

  return (
    <div className="absolute bottom-10 right-10">
      <button onClick={() => setChatBoxOpen(true)}>
        <FontAwesomeIcon icon={faRocket} size="4x" />
      </button>
      <AIChatBox open={chatBoxOpen} onClose={() => setChatBoxOpen(false)} />
    </div>
  );
}