"use strict";
// // memoryStore.ts
// import { BaseStore } from "@langchain/langgraph";
// import { Pool } from 'pg';
// import { pool } from './db/db';
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
exports.memoryStore = exports.PostgresMemoryStore = void 0;
const db_1 = require("./db/db");
class PostgresMemoryStore {
    constructor(pool) {
        this.pool = pool;
    }
    put(namespace, key, value) {
        return __awaiter(this, void 0, void 0, function* () {
            const userId = namespace[0];
            if (!userId) {
                throw new Error("User ID is required for database writes.");
            }
            const client = yield this.pool.connect();
            try {
                yield client.query(`
              INSERT INTO memories (user_id, key, value)
              VALUES ($1, $2, $3)
              ON CONFLICT (user_id, key)
              DO UPDATE SET value = $3, updated_at = CURRENT_TIMESTAMP;
             `, [userId, key, JSON.stringify(value)]);
            }
            catch (error) {
                console.error('Error in put:', error);
            }
            finally {
                client.release();
            }
        });
    }
    get(namespace, key) {
        return __awaiter(this, void 0, void 0, function* () {
            var _a;
            const userId = namespace[0];
            if (!userId) {
                throw new Error("User ID is required for database reads.");
            }
            const client = yield this.pool.connect();
            try {
                const res = yield client.query(`SELECT value FROM memories WHERE user_id = $1 AND key = $2`, [userId, key]);
                return (_a = res.rows[0]) === null || _a === void 0 ? void 0 : _a.value;
            }
            catch (error) {
                console.error("Error in get:", error);
            }
            finally {
                client.release();
            }
        });
    }
    search(namespace) {
        return __awaiter(this, void 0, void 0, function* () {
            const userId = namespace[0];
            if (!userId) {
                throw new Error("User ID is required for database reads.");
            }
            const client = yield this.pool.connect();
            try {
                const res = yield client.query(`SELECT key, value FROM memories WHERE user_id = $1`, [userId]);
                return res.rows.map((row) => ({
                    key: row.key,
                    value: row.value,
                }));
            }
            catch (error) {
                console.error('Error in search:', error);
                return [];
            }
            finally {
                client.release();
            }
        });
    }
    batch(operations) {
        return __awaiter(this, void 0, void 0, function* () {
            console.log("Batch operations are not implemented");
            return Promise.resolve([]);
        });
    }
    delete(namespace, key) {
        return __awaiter(this, void 0, void 0, function* () {
            const userId = namespace[0];
            if (!userId) {
                throw new Error("User ID is required for database deletes.");
            }
            const client = yield this.pool.connect();
            try {
                yield client.query(`DELETE FROM memories WHERE user_id = $1 AND key = $2`, [userId, key]);
            }
            catch (error) {
                console.error("Error in delete:", error);
            }
            finally {
                client.release();
            }
        });
    }
    listNamespaces(options) {
        return __awaiter(this, void 0, void 0, function* () {
            // If you don't need to list all namespaces, you can implement it as a simple returning empty string array
            return Promise.resolve([]);
        });
    }
    start() {
        return __awaiter(this, void 0, void 0, function* () {
            // Perform any startup tasks if required
            console.log("Memory store started.");
        });
    }
    stop() {
        return __awaiter(this, void 0, void 0, function* () {
            // Perform any clean up tasks if required
            console.log("Memory store stopped.");
        });
    }
}
exports.PostgresMemoryStore = PostgresMemoryStore;
exports.memoryStore = new PostgresMemoryStore(db_1.pool);
//# sourceMappingURL=memoryStore.js.map