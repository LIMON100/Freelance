"use strict";
// import { LangGraphRunnableConfig } from "@langchain/langgraph";
// import { z } from "zod";
// import { v4 as uuidv4 } from "uuid";
// import { tool } from "@langchain/core/tools";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
const zod_1 = require("zod");
const tools_1 = require("@langchain/core/tools");
const db_1 = require("../db/db"); // Assuming you have your database pool setup
function upsertPlatformPreference(opts, config) {
    return __awaiter(this, void 0, void 0, function* () {
        var _a;
        const { content } = opts;
        const userId = (_a = config === null || config === void 0 ? void 0 : config.configurable) === null || _a === void 0 ? void 0 : _a.user_id;
        if (!userId) {
            throw new Error("User ID not provided in config");
        }
        const client = yield db_1.pool.connect();
        try {
            // Check if a preference exists for the user
            const checkQuery = `SELECT * FROM user_preferences WHERE user_id = $1`;
            const checkResult = yield client.query(checkQuery, [userId]);
            if (checkResult.rows.length > 0) {
                // Update existing preference
                const updateQuery = `UPDATE user_preferences SET platform_preference = $1 WHERE user_id = $2`;
                yield client.query(updateQuery, [content, userId]);
                return `Updated platform preference to ${content} for user ${userId}`;
            }
            else {
                // Insert new preference
                const insertQuery = `INSERT INTO user_preferences (user_id, platform_preference) VALUES ($1, $2)`;
                yield client.query(insertQuery, [userId, content]);
                return `Stored platform preference ${content} for user ${userId}`;
            }
        }
        catch (error) {
            console.error("Error upserting platform preference:", error);
            throw error;
        }
        finally {
            client.release();
        }
    });
}
const upsertMemoryTool = (0, tools_1.tool)(upsertPlatformPreference, {
    name: "upsertMemory",
    description: "Upsert the user's preferred gaming platform in the database.",
    schema: zod_1.z.object({
        content: zod_1.z.string().describe("The user's preferred gaming platform (e.g., 'pc', 'xbox')."),
        context: zod_1.z.string().describe("Context for the preference (e.g., 'User mentioned this')."),
        memoryId: zod_1.z.string().optional(), // Keep it for compatibility, but we'll use user_id
    }),
});
exports.default = upsertMemoryTool;
