"use strict";

const binding = require("node-gyp-build")(__dirname);

let pollIntervalMs = 1000;
let timer = null;
let lastInfo = null;
let lastError = null;
let lastKey = "";
const listeners = new Set();

function toWebsite(url) {
  if (!url) {
    return undefined;
  }
  try {
    const hasScheme = /^[a-zA-Z][a-zA-Z0-9+.-]*:/.test(url);
    const parsed = new URL(hasScheme ? url : `https://${url}`);
    return parsed.hostname;
  } catch {
    return undefined;
  }
}

function infoKey(info) {
  if (!info) {
    return "";
  }
  return `${info.id ?? ""}|${info.title ?? ""}|${info.url ?? ""}`;
}

function notifyChange(info) {
  for (const listener of listeners) {
    try {
      listener(info);
    } catch {
      // Ignore listener errors to keep polling alive.
    }
  }
}

function fetchActiveWindow() {
  try {
    const info = binding.getActiveWindow();
    if (info && info.url && !info.website) {
      info.website = toWebsite(info.url);
    }
    lastInfo = info;
    lastError = null;
    return info;
  } catch (err) {
    lastError = err;
    return null;
  }
}

function pollOnce() {
  const info = fetchActiveWindow();
  const nextKey = infoKey(info);
  if (nextKey !== lastKey) {
    lastKey = nextKey;
    notifyChange(info);
  }
}

function ensurePolling() {
  if (timer) {
    return;
  }
  startTracking();
}

function startTracking(options = {}) {
  if (timer) {
    return;
  }
  if (typeof options.intervalMs === "number" && options.intervalMs > 0) {
    pollIntervalMs = options.intervalMs;
  }
  timer = setInterval(() => {
    pollOnce();
  }, pollIntervalMs);
  if (typeof timer.unref === "function") {
    timer.unref();
  }
  pollOnce();
}

function stopTracking() {
  if (!timer) {
    return;
  }
  clearInterval(timer);
  timer = null;
}

function configure(options = {}) {
  if (typeof options.intervalMs === "number" && options.intervalMs > 0) {
    pollIntervalMs = options.intervalMs;
  }
  if (timer) {
    stopTracking();
    startTracking({ intervalMs: pollIntervalMs });
  }
}

function getActiveWindow() {
  return fetchActiveWindow();
}

async function getActiveWindowAsync() {
  return fetchActiveWindow();
}

function getLastError() {
  return lastError;
}

function onChange(listener) {
  if (typeof listener !== "function") {
    throw new TypeError("onChange requires a function listener.");
  }
  listeners.add(listener);
  ensurePolling();
  return () => {
    listeners.delete(listener);
  };
}

module.exports = {
  configure,
  startTracking,
  stopTracking,
  getActiveWindow,
  getActiveWindowAsync,
  getLastError,
  onChange
};
