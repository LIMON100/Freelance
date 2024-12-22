import { LangGraphRunnableConfig } from "@langchain/langgraph";
import { z } from "zod";
import { v4 as uuidv4 } from "uuid";
import { tool } from "@langchain/core/tools";

async function upsertMemory(opts: {
  content: string;
  context: string;
  memoryId?: string;
}, config?: LangGraphRunnableConfig): Promise<string> {
  const { content, context, memoryId } = opts;
  if (!config || !config.store) {
    throw new Error("Config or store not provided");
  }

  console.log("opts:", opts);
  console.log("config:", config);

  const memId = memoryId || uuidv4();
  const store = config.store;

  await store.put(["12", "memories"], memId, {
    content,
    context,
  });

  return `Stored memory ${memId}`;
}

const upsertMemoryTool = tool(upsertMemory, {
  name: "upsertMemory",
  description:
    "Upsert a memory in the database related to the user's preferred gaming platform. \
     If an existing platform preference memory is found, update it by passing the same memoryId, \
     rather than creating a new entry. If the user changes or corrects their platform preference, \
     simply update the existing memory with the new information.",
  schema: z.object({
    content: z.string().describe(
      "The user's preferred gaming platform in a single word or short phrase. \
      For example: 'playstation', 'xbox', or 'pc'."
    ),
    context: z.string().describe(
      "Additional context for why or when the user stated this preference. \
       For example: 'User mentioned this after being asked about platform preference.'"
    ),
    memoryId: z
      .string()
      .optional()
      .describe(
        "The memory ID to overwrite if updating an existing preference. \
         For example, using 'platform_preference' ensures future updates overwrite the same memory."
      ),
  }),
});

export default upsertMemoryTool;


// copilot
// import { LangGraphRunnableConfig } from "@langchain/langgraph";
// import { z } from "zod";
// import { v4 as uuidv4 } from "uuid";
// import { tool } from "@langchain/core/tools";

// async function upsertMemory(opts: {
//   content: string;
//   context: string;
//   memoryId?: string;
// }, config?: LangGraphRunnableConfig): Promise<string> {
//   const { content, context, memoryId } = opts;
//   if (!config || !config.store) {
//     throw new Error("Config or store not provided");
//   }

//   console.log("opts:", opts);
//   console.log("config:", config);

//   const memId = memoryId || uuidv4();
//   const store = config.store;

//   await store.put(["12", "memories"], memId, {
//     content,
//     context,
//   });

//   return `Stored memory ${memId}`;
// }

// const upsertMemoryTool = tool(upsertMemory, {
//   name: "upsertMemory",
//   description:
//     "Upsert a memory in the database related to the user's preferred gaming platform. " +
//     "If an existing platform preference memory is found, update it by passing the same memoryId, " +
//     "rather than creating a new entry. If the user changes or corrects their platform preference, " +
//     "simply update the existing memory with the new information.",
//   schema: z.object({
//     content: z.string().describe(
//       "The user's preferred gaming platform in a single word or short phrase. " +
//       "For example: 'playstation', 'xbox', or 'pc'."
//     ),
//     context: z.string().describe(
//       "Additional context for why or when the user stated this preference. " +
//       "For example: 'User mentioned this after being asked about platform preference.'"
//     ),
//     memoryId: z
//       .string()
//       .optional()
//       .describe(
//         "The memory ID to overwrite if updating an existing preference. " +
//         "For example, using 'platform_preference' ensures future updates overwrite the same memory."
//       ),
//   }),
// });

// export default upsertMemoryTool;