import formatSelectedProductsRes from "../utils/formatSelectedProductsRes";
import {tool} from "@langchain/core/tools";
import { pool } from "../db/db";
import {OpenAIEmbeddings} from "@langchain/openai";
import { LangGraphRunnableConfig } from "@langchain/langgraph";
import {z} from "zod";

const getSelectedProduct = async (input: any, config: LangGraphRunnableConfig) => {
  const { query } = input;
  const itemName = query;
  const client = await pool.connect();
  try {
    const embeddings = new OpenAIEmbeddings({
      apiKey: process.env.OPENAI_API_KEY,
      batchSize: 512, // Default value if omitted is 512. Max is 2048
      model: "text-embedding-3-large",
    });

    const store = config?.store;

    const memories = await store?.search(["12"]);

    console.log("platform from memories to fetch ", memories);

    const searchEmbedding = await embeddings.embedQuery(itemName);

    const query = `
            SELECT id, name, platform, price
            FROM products
            ORDER BY embedding <=> ARRAY[${searchEmbedding.toString()}]::vector
          LIMIT 20;
      `;

    const res = await client.query(query);
    return formatSelectedProductsRes(res.rows);
  } catch (err) {
    console.error("Error fetching products:", err);
    throw err;
  } finally {
    client.release();
  }
};

const allProductsTool = tool(async(input, config: LangGraphRunnableConfig) => getSelectedProduct(input, config), {
  name: "all_products",
  description:
    "Fetch product information by name across various platforms, providing a list of games available for the specified platforms.",
  schema: z.object({
    query: z.string().describe("The query to use in your search."),
  }),
});

export default allProductsTool;