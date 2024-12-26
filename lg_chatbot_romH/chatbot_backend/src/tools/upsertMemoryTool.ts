import { LangGraphRunnableConfig } from "@langchain/langgraph";
import { z } from "zod";
import { tool } from "@langchain/core/tools";
import { pool } from "../db/db"; // Assuming you have your database pool setup

async function upsertPlatformPreference(
    opts: {
        content: string;
        context: string;
        memoryId?: string; // We can ignore this now, as we're using user_id
    },
    config?: LangGraphRunnableConfig
): Promise<string> {
    const { content } = opts;
    const userId = config?.configurable?.user_id;

    if (!userId) {
        throw new Error("User ID not provided in config");
    }

    const client = await pool.connect();

    try {
        // Check if a preference exists for the user
        const checkQuery = `SELECT * FROM user_preferences WHERE user_id = $1`;
        const checkResult = await client.query(checkQuery, [userId]);

        if (checkResult.rows.length > 0) {
            // Update existing preference
            const updateQuery = `UPDATE user_preferences SET platform_preference = $1 WHERE user_id = $2`;
            await client.query(updateQuery, [content, userId]);
            return `Updated platform preference to ${content} for user ${userId}`;
        } else {
            // Insert new preference
            const insertQuery = `INSERT INTO user_preferences (user_id, platform_preference) VALUES ($1, $2)`;
            await client.query(insertQuery, [userId, content]);
            return `Stored platform preference ${content} for user ${userId}`;
        }

    } catch (error) {
        console.error("Error upserting platform preference:", error);
        throw error;
    } finally {
        client.release();
    }
}

const upsertMemoryTool = tool(upsertPlatformPreference, {
    name: "upsertMemory",
    description:
        "Upsert the user's preferred gaming platform in the database.",
    schema: z.object({
        content: z.string().describe(
            "The user's preferred gaming platform (e.g., 'pc', 'xbox')."
        ),
        context: z.string().describe(
            "Context for the preference (e.g., 'User mentioned this')."
        ),
        memoryId: z.string().optional(), // Keep it for compatibility, but we'll use user_id
    }),
});

export default upsertMemoryTool;