"use strict";
const fs = require("fs");
const path = require("path");

const MEMORY_FILE = path.join(__dirname, "memory.json");

function loadMemory() {
  if (fs.existsSync(MEMORY_FILE)) {
    const data = fs.readFileSync(MEMORY_FILE, "utf-8");
    return JSON.parse(data);
  }
  return {};
}

function saveMemory(memory) {
  fs.writeFileSync(MEMORY_FILE, JSON.stringify(memory, null, 2));
}

module.exports = { loadMemory, saveMemory };