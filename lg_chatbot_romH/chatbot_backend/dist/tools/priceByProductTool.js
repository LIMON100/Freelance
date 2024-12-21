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
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const db_1 = require("../db/db");
const openai_1 = require("@langchain/openai");
const getPriceByProductRes_1 = __importDefault(require("../utils/getPriceByProductRes"));
const tools_1 = require("@langchain/core/tools");
const zod_1 = require("zod");
const getPriceByProduct = (input) => __awaiter(void 0, void 0, void 0, function* () {
    const { query } = input;
    const itemName = query;
    const client = yield db_1.pool.connect();
    try {
        const embeddings = new openai_1.OpenAIEmbeddings({
            apiKey: process.env.OPENAI_API_KEY,
            batchSize: 512, // Default value if omitted is 512. Max is 2048
            model: "text-embedding-3-large",
        });
        const searchEmbedding = yield embeddings.embedQuery(itemName);
        const query = `
        SELECT id, name, price
        FROM products
        ORDER BY embedding <=> ARRAY[${searchEmbedding.toString()}]::vector
          LIMIT 20;
      `;
        const res = yield client.query(query);
        return (0, getPriceByProductRes_1.default)(res.rows);
    }
    catch (err) {
        console.error("Error fetching products:", err);
        throw err;
    }
    finally {
        client.release();
    }
});
const priceByProductTool = (0, tools_1.tool)((input) => __awaiter(void 0, void 0, void 0, function* () { return getPriceByProduct(input); }), {
    name: "price_by_product_name",
    description: "Fetch product information about the price by name.",
    schema: zod_1.z.object({
        query: zod_1.z.string().describe("The query to use in your search."),
    }),
});
exports.default = priceByProductTool;
//# sourceMappingURL=priceByProductTool.js.map