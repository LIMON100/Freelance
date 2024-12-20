"use strict";
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
const uuid_1 = require("uuid");
const tools_1 = require("@langchain/core/tools");
function upsertMemory(opts, config) {
    return __awaiter(this, void 0, void 0, function* () {
        const { content, context, memoryId } = opts;
        if (!config || !config.store) {
            throw new Error("Config or store not provided");
        }
        console.log("opts:", opts);
        console.log("config:", config);
        const memId = memoryId || (0, uuid_1.v4)();
        const store = config.store;
        yield store.put(["12", "memories"], memId, {
            content,
            context,
        });
        return `Stored memory ${memId}`;
    });
}
const upsertMemoryTool = (0, tools_1.tool)(upsertMemory, {
    name: "upsertMemory",
    description: "Upsert a memory in the database related to the user's preferred gaming platform. \
     If an existing platform preference memory is found, update it by passing the same memoryId, \
     rather than creating a new entry. If the user changes or corrects their platform preference, \
     simply update the existing memory with the new information.",
    schema: zod_1.z.object({
        content: zod_1.z.string().describe("The user's preferred gaming platform in a single word or short phrase. \
      For example: 'playstation', 'xbox', or 'pc'."),
        context: zod_1.z.string().describe("Additional context for why or when the user stated this preference. \
       For example: 'User mentioned this after being asked about platform preference.'"),
        memoryId: zod_1.z
            .string()
            .optional()
            .describe("The memory ID to overwrite if updating an existing preference. \
         For example, using 'platform_preference' ensures future updates overwrite the same memory."),
    }),
});
exports.default = upsertMemoryTool;
