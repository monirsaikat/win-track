"use strict";

const isWindows = process.platform === "win32";
let binding = null;
let activeWinModule = null;
let lastError = null;

function loadBinding() {
  if (!isWindows) {
    return null;
  }
  if (binding) {
    return binding;
  }
  try {
    binding = require("node-gyp-build")(__dirname);
    return binding;
  } catch (err) {
    lastError = err;
    return null;
  }
}

async function loadActiveWin() {
  if (activeWinModule) {
    return activeWinModule;
  }
  try {
    activeWinModule = require("active-win");
    return activeWinModule;
  } catch (err) {
    try {
      activeWinModule = await import("active-win");
      return activeWinModule;
    } catch (importErr) {
      lastError = importErr;
      return null;
    }
  }
}

function resolveActiveWin(mod) {
  if (!mod) {
    return null;
  }
  if (typeof mod === "function") {
    return mod;
  }
  if (typeof mod.default === "function") {
    return mod.default;
  }
  if (typeof mod.activeWin === "function") {
    return mod.activeWin;
  }
  return null;
}

function resolveActiveWinSync(mod, activeWin) {
  if (mod && typeof mod.sync === "function") {
    return mod.sync;
  }
  if (activeWin && typeof activeWin.sync === "function") {
    return activeWin.sync;
  }
  return null;
}

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

function normalizeInfo(info) {
  if (info && info.url && !info.website) {
    info.website = toWebsite(info.url);
  }
  return info;
}

function fetchActiveWindowSync() {
  const native = loadBinding();
  if (native) {
    try {
      const info = native.getActiveWindow();
      lastError = null;
      return normalizeInfo(info);
    } catch (err) {
      lastError = err;
      return null;
    }
  }

  if (!activeWinModule) {
    try {
      activeWinModule = require("active-win");
    } catch (err) {
      lastError = err;
    }
  }

  if (!activeWinModule) {
    lastError = new Error(
      "Active window sync is not available yet on this platform. Use getActiveWindowAsync()."
    );
    return null;
  }

  const activeWin = resolveActiveWin(activeWinModule);
  const activeWinSync = resolveActiveWinSync(activeWinModule, activeWin);
  if (!activeWinSync) {
    lastError = new Error(
      "Active window sync is not supported by the current platform module. Use getActiveWindowAsync()."
    );
    return null;
  }

  try {
    const info = activeWinSync();
    lastError = null;
    return normalizeInfo(info);
  } catch (err) {
    lastError = err;
    return null;
  }
}

async function fetchActiveWindowAsync() {
  const native = loadBinding();
  if (native) {
    try {
      const info = native.getActiveWindow();
      lastError = null;
      return normalizeInfo(info);
    } catch (err) {
      lastError = err;
      return null;
    }
  }

  const mod = await loadActiveWin();
  const activeWin = resolveActiveWin(mod);
  if (!activeWin) {
    lastError =
      lastError ||
      new Error("Active window module is unavailable on this platform.");
    return null;
  }

  try {
    const info = await activeWin();
    lastError = null;
    return normalizeInfo(info);
  } catch (err) {
    lastError = err;
    return null;
  }
}

function getActiveWindow() {
  return fetchActiveWindowSync();
}

async function getActiveWindowAsync() {
  return fetchActiveWindowAsync();
}

function getLastError() {
  return lastError;
}

module.exports = {
  getActiveWindow,
  getActiveWindowAsync,
  getLastError
};
