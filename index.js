"use strict";

const binding = require("node-gyp-build")(__dirname);

let lastError = null;

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

function fetchActiveWindow() {
  try {
    const info = binding.getActiveWindow();
    if (info && info.url && !info.website) {
      info.website = toWebsite(info.url);
    }
    lastError = null;
    return info;
  } catch (err) {
    lastError = err;
    return null;
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

module.exports = {
  getActiveWindow,
  getActiveWindowAsync,
  getLastError
};
