"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const formatPriceByProductRes = (rows) => {
    if (!rows || rows.length === 0) {
        return "No games found for the given query.";
    }
    return rows.map((row) => `*${row.name}* (Price: ${row.price})`).join("\n");
};
exports.default = formatPriceByProductRes;
