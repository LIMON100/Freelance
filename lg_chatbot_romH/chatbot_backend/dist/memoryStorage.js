"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.loadMemory = loadMemory;
exports.saveMemory = saveMemory;
const fs_1 = __importDefault(require("fs"));
const path_1 = __importDefault(require("path"));
const MEMORY_FILE = path_1.default.join(__dirname, "memory.json");
function loadMemory() {
    if (fs_1.default.existsSync(MEMORY_FILE)) {
        const data = fs_1.default.readFileSync(MEMORY_FILE, "utf-8");
        return JSON.parse(data);
    }
    return {};
}
function saveMemory(memory) {
    fs_1.default.writeFileSync(MEMORY_FILE, JSON.stringify(memory, null, 2));
}
//# sourceMappingURL=memoryStorage.js.map