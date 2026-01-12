import createModule from "./ficl_wasm.js";

const outputEl = document.getElementById("output");
const inputEl = document.getElementById("input");
const stackEl = document.getElementById("stack");
const statusEl = document.getElementById("status");
const clearBtn = document.getElementById("clear-btn");
const resetBtn = document.getElementById("reset-btn");

let Module = null;
let history = [];
let historyIndex = -1;

function appendOutput(text) {
  if (!text) return;
  outputEl.textContent += text;
  outputEl.scrollTop = outputEl.scrollHeight;
}

function setStatus(text) {
  statusEl.textContent = text;
}

/* Map the low 8 bits of value to the LED row. */
function setLedBits(value) {
  const leds = document.querySelectorAll(".led-row .led");
  const bits = Number(value) >>> 0;
  const count = Math.min(leds.length, 8);
  for (let i = 0; i < count; i += 1) {
    const isOn = (bits >> i) & 1;
    leds[i].classList.toggle("is-on", Boolean(isOn));
  }
}

window.setLedBits = setLedBits;

/* Schedule a repaint so long-running words can yield. */
function requestRefresh() {
  if (typeof window.requestAnimationFrame === "function") {
    window.requestAnimationFrame(() => {});
  }
}

window.requestRefresh = requestRefresh;

function updateStack() {
  if (!Module) return;
  const bufSize = 256;
  const base = Module.stackSave();
  const ptr = Module.stackAlloc(bufSize);
  Module._ficlWasmStackHex(ptr, bufSize, 8);
  const stackText = Module.UTF8ToString(ptr);
  Module.stackRestore(base);
  stackEl.textContent = (stackText || "").trim();
  stackEl.classList.remove("flash");
  void stackEl.offsetWidth;
  stackEl.classList.add("flash");
}

function runLine(line) {
  if (!Module) return;
  if (!line.trim()) return;

  appendOutput(`${line}\n`);

  const len = Module.lengthBytesUTF8(line) + 1;
  const base = Module.stackSave();
  const ptr = Module.stackAlloc(len);
  Module.stringToUTF8(line, ptr, len);
  const rc = Module._ficlWasmEval(ptr);
  Module.stackRestore(base);

  const outPtr = Module._ficlWasmGetOutput();
  const outLen = Module._ficlWasmGetOutputLen();
  const output = outLen ? Module.UTF8ToString(outPtr, outLen) : "";
  if (output) appendOutput(output);
  Module._ficlWasmClearOutput();

  updateStack();
  setStatus(' ');
}

function handleKeydown(event) {
  if (event.key === "Enter") {
    const line = inputEl.value;
    inputEl.value = "";
    if (line.trim()) {
      history.push(line);
      historyIndex = history.length;
    }
    runLine(line);
  } else if (event.key === "ArrowUp") {
    if (history.length === 0) return;
    historyIndex = Math.max(0, historyIndex - 1);
    inputEl.value = history[historyIndex] || "";
    setTimeout(() => inputEl.setSelectionRange(inputEl.value.length, inputEl.value.length), 0);
  } else if (event.key === "ArrowDown") {
    if (history.length === 0) return;
    historyIndex = Math.min(history.length, historyIndex + 1);
    inputEl.value = history[historyIndex] || "";
  }
}

clearBtn.addEventListener("click", () => {
  outputEl.textContent = "";
});

resetBtn.addEventListener("click", () => {
  if (!Module) return;
  Module._ficlWasmReset();
  Module._ficlWasmClearOutput();
  updateStack();
  setStatus("VM reset");
});

inputEl.addEventListener("keydown", handleKeydown);

async function init() {
  try {
    Module = await createModule();
    Module._ficlWasmInit(20000, 256);
    const outPtr = Module._ficlWasmGetOutput();
    const outLen = Module._ficlWasmGetOutputLen();
    const output = outLen ? Module.UTF8ToString(outPtr, outLen) : "";
    if (output) appendOutput(output);
    Module._ficlWasmClearOutput();
    updateStack();
    setStatus("Ready");
    inputEl.focus();
  } catch (err) {
    setStatus("Failed to load WASM");
    appendOutput(String(err) + "\n");
  }
}

init();
