import { pool } from "../db/db";
import { OpenAIEmbeddings } from "@langchain/openai";
import formatPriceByProductRes from "../utils/getPriceByProductRes";
import { tool } from "@langchain/core/tools";
import { z } from "zod";

const getPriceByProduct = async (input: any) => {
  const { query } = input;
  const itemName = query;
  const client = await pool.connect();
  try {
    const embeddings = new OpenAIEmbeddings({
      apiKey: process.env.OPENAI_API_KEY,
      batchSize: 512, // Default value if omitted is 512. Max is 2048
      model: "text-embedding-3-large",
    });

    const searchEmbedding = await embeddings.embedQuery(itemName);

    const query = `
        SELECT id, name, price
        FROM products
        ORDER BY embedding <=> ARRAY[${searchEmbedding.toString()}]::vector
          LIMIT 20;
      `;

    const res = await client.query(query);
    return formatPriceByProductRes(res.rows);
  } catch (err) {
    console.error("Error fetching products:", err);
    throw err;
  } finally {
    client.release();
  }
};

const priceByProductTool = tool(async(input) => getPriceByProduct(input), {
  name: "price_by_product_name",
  description: "Fetch product information about the price by name.",
  schema: z.object({
    query: z.string().describe("The query to use in your search."),
  }),
});

export default priceByProductTool;