"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.ConfigurationAnnotation = void 0;
exports.ensureConfiguration = ensureConfiguration;
const langgraph_1 = require("@langchain/langgraph");
const prompts_1 = require("../prompts/prompts");
exports.ConfigurationAnnotation = langgraph_1.Annotation.Root({
    messages: (0, langgraph_1.Annotation)({
        reducer: (x, y) => x.concat(y),
    }),
    userId: (0, langgraph_1.Annotation)(),
    model: (0, langgraph_1.Annotation)(),
    systemPrompt: (0, langgraph_1.Annotation)(),
});
function ensureConfiguration(config) {
    const configurable = (config === null || config === void 0 ? void 0 : config.configurable) || {};
    return {
        userId: (configurable === null || configurable === void 0 ? void 0 : configurable.userId) || "default",
        model: (configurable === null || configurable === void 0 ? void 0 : configurable.model) || "anthropic/claude-3-5-sonnet-20240620",
        systemPrompt: (configurable === null || configurable === void 0 ? void 0 : configurable.systemPrompt) || prompts_1.SYSTEM_PROMPT,
    };
}
//# sourceMappingURL=configuration.js.map