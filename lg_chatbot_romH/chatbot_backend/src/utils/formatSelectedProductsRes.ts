type FormatSelectedProductsRes = {
  id: string,
  name: string,
  platform: string,
  price: string,
}[]


const formatSelectedProductsRes = (rows: FormatSelectedProductsRes) => {
  if (!rows || rows.length === 0) {
    return "No games found for the given query.";
  }

  return rows.map(
    (row, price) => `*${row.name}* (Platform: ${row.platform}) (Price: ${row.price}`
  ).join("\n");
}

export default formatSelectedProductsRes;