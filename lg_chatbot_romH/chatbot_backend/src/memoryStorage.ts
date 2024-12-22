import fs from "fs";
import path from "path";

const MEMORY_FILE = path.join(__dirname, "memory.json");

export function loadMemory(): Record<string, any> {
  if (fs.existsSync(MEMORY_FILE)) {
    const data = fs.readFileSync(MEMORY_FILE, "utf-8");
    return JSON.parse(data);
  }
  return {};
}

export function saveMemory(memory: Record<string, any>): void {
  fs.writeFileSync(MEMORY_FILE, JSON.stringify(memory, null, 2));
}