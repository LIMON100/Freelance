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
const express_1 = __importDefault(require("express"));
const cors_1 = __importDefault(require("cors"));
const body_parser_1 = __importDefault(require("body-parser"));
const db_1 = require("./db/db");
const chatBot_1 = __importDefault(require("./chatBot"));
require('dotenv').config();
const app = (0, express_1.default)();
app.use((0, cors_1.default)({
    origin: 'http://localhost:3000',
}));
app.use(body_parser_1.default.json());
const PORT = 5002;
app.post('/api/chat', (req, res) => __awaiter(void 0, void 0, void 0, function* () {
    const userMessage = req.body.message;
    const prevMessages = req.body.prevMessages;
    const resultAIChatbot = yield (0, chatBot_1.default)(userMessage, prevMessages);
    const botResponse = { role: 'assistant', content: resultAIChatbot };
    res.json(botResponse);
}));
app.listen(PORT, () => {
    console.log(`Backend server running on http://localhost:${PORT}`);
});
(0, db_1.testConnection)();
