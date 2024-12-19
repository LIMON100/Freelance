import { Annotation, LangGraphRunnableConfig } from "@langchain/langgraph";
import { SYSTEM_PROMPT } from "../prompts/prompts";
import { BaseMessage } from "@langchain/core/messages";

export const ConfigurationAnnotation = Annotation.Root({
  messages: Annotation<BaseMessage[]>({
    reducer: (x, y) => x.concat(y),
  }),
  userId: Annotation<string>(),
  model: Annotation<string>(),
  systemPrompt: Annotation<string>(),
});

export type Configuration = typeof ConfigurationAnnotation.State;

export function ensureConfiguration(config?: LangGraphRunnableConfig) {
  const configurable = config?.configurable || {};
  return {
    userId: configurable?.userId || "default",
    model: configurable?.model || "anthropic/claude-3-5-sonnet-20240620",
    systemPrompt: configurable?.systemPrompt || SYSTEM_PROMPT,
  };
}