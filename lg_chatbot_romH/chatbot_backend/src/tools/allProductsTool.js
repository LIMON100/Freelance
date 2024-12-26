"use strict";
// import formatSelectedProductsRes from "../utils/formatSelectedProductsRes";
// import {tool} from "@langchain/core/tools";
// import { pool } from "../db/db";
// import {OpenAIEmbeddings} from "@langchain/openai";
// import { LangGraphRunnableConfig } from "@langchain/langgraph";
// import {z} from "zod";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
// const getSelectedProduct = async (input: any, config: LangGraphRunnableConfig) => {
//   const { query } = input;
//   const itemName = query;
//   const client = await pool.connect();
//   try {
//     const embeddings = new OpenAIEmbeddings({
//       apiKey: process.env.OPENAI_API_KEY,
//       batchSize: 512, // Default value if omitted is 512. Max is 2048
//       model: "text-embedding-3-large",
//     });
//     const store = config?.store;
//     const memories = await store?.search(["12"]);
//     console.log("platform from memories to fetch ", memories);
//     const searchEmbedding = await embeddings.embedQuery(itemName);
//     const query = `
//             SELECT id, name, platform, price
//             FROM products
//             ORDER BY embedding <=> ARRAY[${searchEmbedding.toString()}]::vector
//           LIMIT 20;
//       `;
//     const res = await client.query(query);
//     return formatSelectedProductsRes(res.rows);
//   } catch (err) {
//     console.error("Error fetching products:", err);
//     throw err;
//   } finally {
//     client.release();
//   }
// };
// const allProductsTool = tool(async(input, config: LangGraphRunnableConfig) => getSelectedProduct(input, config), {
//   name: "all_products",
//   description:
//     "Fetch product information by name across various platforms, providing a list of games available for the specified platforms.",
//   schema: z.object({
//     query: z.string().describe("The query to use in your search."),
//   }),
// });
// export default allProductsTool;
const formatSelectedProductsRes_1 = __importDefault(require("../utils/formatSelectedProductsRes"));
const tools_1 = require("@langchain/core/tools");
const db_1 = require("../db/db");
const openai_1 = require("@langchain/openai");
const zod_1 = require("zod");
const getSelectedProduct = (input, config) => __awaiter(void 0, void 0, void 0, function* () {
    const { query } = input;
    const itemName = query;
    const client = yield db_1.pool.connect();
    try {
        const embeddings = new openai_1.OpenAIEmbeddings({
            apiKey: process.env.OPENAI_API_KEY,
            batchSize: 512, // Default value if omitted is 512. Max is 2048
            model: "text-embedding-3-large",
        });
        const store = config === null || config === void 0 ? void 0 : config.store;
        const memories = yield (store === null || store === void 0 ? void 0 : store.search(["12"]));
        console.log("platform from memories to fetch ", memories);
        const searchEmbedding = yield embeddings.embedQuery(itemName);
        const query = `
            SELECT id, name, platform, price
            FROM products
            ORDER BY embedding <=> ARRAY[${searchEmbedding.toString()}]::vector
          LIMIT 20;
      `;
        // console.log("Executing Query:", query); // Log the query
        const res = yield client.query(query); // The database query line
        console.log("Query Result:", res);
        return (0, formatSelectedProductsRes_1.default)(res.rows);
    }
    catch (err) {
        console.error("Error fetching products:", err);
        throw err;
    }
    finally {
        client.release();
    }
});
const allProductsTool = (0, tools_1.tool)((input, config) => __awaiter(void 0, void 0, void 0, function* () { return getSelectedProduct(input, config); }), {
    name: "all_products",
    description: "Fetch product information by name across various platforms, providing a list of games available for the specified platforms.",
    schema: zod_1.z.object({
        query: zod_1.z.string().describe("The query to use in your search."),
    }),
});
exports.default = allProductsTool;
