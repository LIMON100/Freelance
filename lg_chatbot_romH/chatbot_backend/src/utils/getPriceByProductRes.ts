type FormatPriceByProductRes = {
  id: string,
  name: string,
  price: string,
}[]

const formatPriceByProductRes = (rows: FormatPriceByProductRes) => {
  if (!rows || rows.length === 0) {
    return "No games found for the given query.";
  }

  return rows.map(
    (row) => `*${row.name}* (Price: ${row.price})`
  ).join("\n");
}

export default formatPriceByProductRes;