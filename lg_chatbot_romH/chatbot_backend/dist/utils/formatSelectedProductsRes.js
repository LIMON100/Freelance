"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const formatSelectedProductsRes = (rows) => {
    if (!rows || rows.length === 0) {
        return "No games found for the given query.";
    }
    return rows.map((row, price) => `*${row.name}* (Platform: ${row.platform}) (Price: ${row.price}`).join("\n");
};
exports.default = formatSelectedProductsRes;
//# sourceMappingURL=formatSelectedProductsRes.js.map