import { Message, useChat } from "ai/react";
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faXmark, faRocket, faCircleXmark} from '@fortawesome/free-solid-svg-icons'
import Link from "next/link";
import { useEffect, useRef, useState } from "react";
import ReactMarkdown from "react-markdown";
import { cn } from "@/app/lib/utils";
import { v4 as uuidv4 } from "uuid";

interface AIChatBoxProps {
  open: boolean;
  onClose: () => void;
}

export default function AIChatBox({ open, onClose }: AIChatBoxProps) {
  const {
    messages,
    input,
    handleInputChange,
    // handleSubmit,
    setMessages,
    // isLoading,
    error,
  } = useChat();

  const [isLoading, setIsLoading] = useState(false);

  const inputRef = useRef<HTMLInputElement>(null);
  const scrollRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [messages]);

  useEffect(() => {
    if (open) {
      inputRef.current?.focus();
    }
  }, [open]);

  const lastMessageIsUser = messages[messages.length - 1]?.role === "user";

  const customHandleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();

    try {
      // Send data to the backend API
      setIsLoading(true);

      const response = await fetch('http://localhost:5002/api/chat', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ message: input, prevMessages: messages })
      });

      const assistant = await response.json();

      setMessages((prev) => [
        ...prev,
        { id: uuidv4(), role: "user", content: input },
        { id: uuidv4(), role: "assistant", content: assistant.content },
      ]);

      handleInputChange({ target: { value: "" } });
    } catch (err) {
      console.error("Error submitting form:", err);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div
      className={cn(
        "bottom-0 right-0 z-50 w-full max-w-[500px] p-1 xl:right-5 xl:bottom-5",
        open ? "fixed" : "hidden",
      )}
    >
      <button onClick={onClose} className="mt-4 mr-4 ms-auto end-0 absolute">
        <FontAwesomeIcon icon={faCircleXmark} size="xl" className="rounded-full bg-background"/>
      </button>
      <div className="flex h-[700px] flex-col rounded border bg-background shadow-xl">
        <div className="mt-12 h-full overflow-y-auto px-3" ref={scrollRef}>
          {messages.map((message) => (
            <ChatMessage message={message} key={message.id} />
          ))}
          {/*{isLoading && lastMessageIsUser && (*/}
          {isLoading && (
            <ChatMessage
              message={{
                id: "loading",
                role: "assistant",
                content: "Loading...",
              }}
            />
          )}
          {error && (
            <ChatMessage
              message={{
                id: "error",
                role: "assistant",
                content: "Something went wrong. Please try again!",
              }}
            />
          )}
          {!error && messages.length === 0 && (
            <div className="mx-8 flex h-full flex-col items-center justify-center gap-3 text-center">
              <p className="text-lg font-medium">
                Send a message to start!
              </p>
            </div>
          )}
        </div>
        <form onSubmit={customHandleSubmit} className="m-3 flex gap-1">
          <button
            type="button"
            className="flex w-10 flex-none items-center justify-center"
            title="Clear chat"
            onClick={() => setMessages([])}
          >
            <FontAwesomeIcon icon={faXmark} size="lg" />
          </button>
          <input
            value={input}
            onChange={handleInputChange}
            placeholder="Type something..."
            className="grow rounded border bg-background px-3 py-2"
            ref={inputRef}
          />
          <button
            type="submit"
            className="flex w-10 flex-none items-center justify-center bg-green-500 disabled:opacity-50"
            disabled={input.length === 0 || isLoading}
            title="Submit message"
          >
            Send!
          </button>
        </form>
      </div>
    </div>
  );
}

interface ChatMessageProps {
  message: Message;
}

function ChatMessage({ message: { role, content } }: ChatMessageProps) {
  const isAiMessage = role === "assistant";

  return (
    <div
      className={cn(
        "mb-3 flex items-center",
        isAiMessage ? "me-5 justify-start" : "ms-5 justify-end",
      )}
    >
      {isAiMessage && (<FontAwesomeIcon icon={faRocket} size="lg" className="mr-2 flex-none" />)}
      <div
        className={cn(
          "rounded-md border px-3 py-2",
          isAiMessage ? "bg-background" : "bg-yellow-500 text-background",
        )}
      >
        <ReactMarkdown
          components={{
            a: ({ node, ref, ...props }) => (
              <Link
                {...props}
                href={props.href ?? ""}
                className="text-primary hover:underline"
              />
            ),
            p: ({ node, ...props }) => (
              <p {...props} className="mt-3 first:mt-0" />
            ),
            ul: ({ node, ...props }) => (
              <ul
                {...props}
                className="mt-3 list-inside list-disc first:mt-0"
              />
            ),
            li: ({ node, ...props }) => <li {...props} className="mt-1" />,
          }}
        >
          {content}
        </ReactMarkdown>
      </div>
    </div>
  );
}