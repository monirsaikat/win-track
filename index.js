"use strict";

const isWindows = process.platform === "win32";
const isMac = process.platform === "darwin";
const isLinux = process.platform === "linux";
const childProcess = require("child_process");
const fs = require("fs");
let binding = null;
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

function runCommandSync(command, args, options = {}) {
  try {
    return childProcess.execFileSync(command, args, {
      encoding: "utf8",
      timeout: 1500,
      windowsHide: true,
      ...options
    }).trim();
  } catch (err) {
    lastError = err;
    return null;
  }
}

function runCommandAsync(command, args, options = {}) {
  return new Promise((resolve) => {
    childProcess.execFile(
      command,
      args,
      { encoding: "utf8", timeout: 1500, windowsHide: true, ...options },
      (err, stdout) => {
        if (err) {
          lastError = err;
          resolve(null);
          return;
        }
        resolve(stdout.trim());
      }
    );
  });
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

const MAC_BROWSER_URL_SCRIPTS = {
  "google chrome":
    'tell application "Google Chrome" to get URL of active tab of front window',
  "brave browser":
    'tell application "Brave Browser" to get URL of active tab of front window',
  "microsoft edge":
    'tell application "Microsoft Edge" to get URL of active tab of front window',
  safari: 'tell application "Safari" to get URL of current tab of front window',
  firefox:
    'tell application "System Events" to tell process "Firefox" to get value of attribute "AXDocument" of front window'
};

function normalizeMacUrl(output) {
  if (!output) {
    return undefined;
  }
  const value = output.trim();
  if (!value || value === "missing value") {
    return undefined;
  }
  return value;
}

function normalizeMacAppName(appName) {
  if (!appName) {
    return "";
  }
  return appName.trim().toLowerCase();
}

function readMacBrowserUrlSync(appName) {
  const script = MAC_BROWSER_URL_SCRIPTS[normalizeMacAppName(appName)];
  if (!script) {
    return undefined;
  }
  const output = runCommandSync("osascript", ["-e", script]);
  return normalizeMacUrl(output);
}

async function readMacBrowserUrlAsync(appName) {
  const script = MAC_BROWSER_URL_SCRIPTS[normalizeMacAppName(appName)];
  if (!script) {
    return undefined;
  }
  const output = await runCommandAsync("osascript", ["-e", script]);
  return normalizeMacUrl(output);
}

function normalizeInfo(info) {
  if (info && info.url && !info.website) {
    info.website = toWebsite(info.url);
  }
  return info;
}

function parseLinuxActiveWindowId(output) {
  if (!output) {
    return null;
  }
  const match = output.match(/0x[0-9a-fA-F]+/);
  return match ? match[0] : null;
}

function parseLinuxXprop(output) {
  if (!output) {
    return {};
  }
  const info = {};
  const titleMatch =
    output.match(/_NET_WM_NAME\(\w+\)\s+=\s+"(.*)"/) ||
    output.match(/WM_NAME\(\w+\)\s+=\s+"(.*)"/);
  if (titleMatch) {
    info.title = titleMatch[1];
  }
  const classMatch = output.match(/WM_CLASS\(\w+\)\s+=\s+"([^"]+)",\s+"([^"]+)"/);
  if (classMatch) {
    info.appName = classMatch[2] || classMatch[1];
    info.ownerName = classMatch[2] || classMatch[1];
  }
  const pidMatch = output.match(/_NET_WM_PID\(\w+\)\s+=\s+(\d+)/);
  if (pidMatch) {
    info.processId = Number(pidMatch[1]);
  }
  return info;
}

function readLinuxBounds(windowId) {
  const output = runCommandSync("xwininfo", ["-id", windowId]);
  if (!output) {
    return undefined;
  }
  const xMatch = output.match(/Absolute upper-left X:\s+(-?\d+)/);
  const yMatch = output.match(/Absolute upper-left Y:\s+(-?\d+)/);
  const wMatch = output.match(/Width:\s+(\d+)/);
  const hMatch = output.match(/Height:\s+(\d+)/);
  if (!xMatch || !yMatch || !wMatch || !hMatch) {
    return undefined;
  }
  return {
    x: Number(xMatch[1]),
    y: Number(yMatch[1]),
    width: Number(wMatch[1]),
    height: Number(hMatch[1])
  };
}

function readLinuxProcessPath(processId) {
  if (!processId) {
    return undefined;
  }
  try {
    return fs.readlinkSync(`/proc/${processId}/exe`);
  } catch (err) {
    lastError = err;
    return undefined;
  }
}

function readLinuxActiveWindowSync() {
  const activeOutput = runCommandSync("xprop", ["-root", "_NET_ACTIVE_WINDOW"]);
  const windowId = parseLinuxActiveWindowId(activeOutput);
  if (!windowId || windowId === "0x0") {
    return null;
  }
  const details = runCommandSync("xprop", [
    "-id",
    windowId,
    "_NET_WM_NAME",
    "WM_NAME",
    "WM_CLASS",
    "_NET_WM_PID"
  ]);
  const parsed = parseLinuxXprop(details);
  const bounds = readLinuxBounds(windowId);
  const path = readLinuxProcessPath(parsed.processId);
  return normalizeInfo({
    appName: parsed.appName,
    title: parsed.title,
    id: windowId,
    bounds,
    owner: {
      name: parsed.ownerName || parsed.appName,
      path,
      processId: parsed.processId
    },
    url: undefined,
    website: undefined
  });
}

async function readLinuxActiveWindowAsync() {
  const activeOutput = await runCommandAsync("xprop", [
    "-root",
    "_NET_ACTIVE_WINDOW"
  ]);
  const windowId = parseLinuxActiveWindowId(activeOutput);
  if (!windowId || windowId === "0x0") {
    return null;
  }
  const details = await runCommandAsync("xprop", [
    "-id",
    windowId,
    "_NET_WM_NAME",
    "WM_NAME",
    "WM_CLASS",
    "_NET_WM_PID"
  ]);
  const parsed = parseLinuxXprop(details);
  const bounds = readLinuxBounds(windowId);
  const path = readLinuxProcessPath(parsed.processId);
  return normalizeInfo({
    appName: parsed.appName,
    title: parsed.title,
    id: windowId,
    bounds,
    owner: {
      name: parsed.ownerName || parsed.appName,
      path,
      processId: parsed.processId
    },
    url: undefined,
    website: undefined
  });
}

function parseMacLines(output) {
  if (!output) {
    return {};
  }
  const lines = output.split(/\r?\n/);
  const info = {};
  for (const line of lines) {
    const [key, ...rest] = line.split(":");
    const value = rest.join(":").trim();
    if (key === "app") {
      info.appName = value || undefined;
    } else if (key === "title") {
      info.title = value || undefined;
    } else if (key === "pid") {
      info.processId = Number(value);
    } else if (key === "bounds") {
      const parts = value.split(",").map((part) => Number(part.trim()));
      if (parts.length === 4 && parts.every((n) => Number.isFinite(n))) {
        info.bounds = {
          x: parts[0],
          y: parts[1],
          width: parts[2],
          height: parts[3]
        };
      }
    }
  }
  return info;
}

function readMacProcessPath(processId) {
  if (!processId) {
    return undefined;
  }
  const output = runCommandSync("ps", ["-p", String(processId), "-o", "command="]);
  if (!output) {
    return undefined;
  }
  const parts = output.split(" ");
  return parts[0] || undefined;
}

function readMacActiveWindowSync() {
  const script = [
    'tell application "System Events"',
    'set frontApp to first application process whose frontmost is true',
    'set appName to name of frontApp',
    'set appPid to unix id of frontApp',
    'set windowTitle to ""',
    'set boundsInfo to ""',
    'try',
    'set windowTitle to name of front window of frontApp',
    'set {xPos, yPos} to position of front window of frontApp',
    'set {wSize, hSize} to size of front window of frontApp',
    'set boundsInfo to (xPos as text) & "," & (yPos as text) & "," & (wSize as text) & "," & (hSize as text)',
    'end try',
    'return "app:" & appName & "\n" & "title:" & windowTitle & "\n" & "pid:" & appPid & "\n" & "bounds:" & boundsInfo',
    "end tell"
  ].join("\n");
  const output = runCommandSync("osascript", ["-e", script]);
  const parsed = parseMacLines(output);
  const path = readMacProcessPath(parsed.processId);
  const url = readMacBrowserUrlSync(parsed.appName);
  return normalizeInfo({
    appName: parsed.appName,
    title: parsed.title,
    id: parsed.processId ? String(parsed.processId) : undefined,
    bounds: parsed.bounds,
    owner: {
      name: parsed.appName,
      path,
      processId: parsed.processId
    },
    url,
    website: undefined
  });
}

async function readMacActiveWindowAsync() {
  const script = [
    'tell application "System Events"',
    'set frontApp to first application process whose frontmost is true',
    'set appName to name of frontApp',
    'set appPid to unix id of frontApp',
    'set windowTitle to ""',
    'set boundsInfo to ""',
    'try',
    'set windowTitle to name of front window of frontApp',
    'set {xPos, yPos} to position of front window of frontApp',
    'set {wSize, hSize} to size of front window of frontApp',
    'set boundsInfo to (xPos as text) & "," & (yPos as text) & "," & (wSize as text) & "," & (hSize as text)',
    'end try',
    'return "app:" & appName & "\n" & "title:" & windowTitle & "\n" & "pid:" & appPid & "\n" & "bounds:" & boundsInfo',
    "end tell"
  ].join("\n");
  const output = await runCommandAsync("osascript", ["-e", script]);
  const parsed = parseMacLines(output);
  const path = readMacProcessPath(parsed.processId);
  const url = await readMacBrowserUrlAsync(parsed.appName);
  return normalizeInfo({
    appName: parsed.appName,
    title: parsed.title,
    id: parsed.processId ? String(parsed.processId) : undefined,
    bounds: parsed.bounds,
    owner: {
      name: parsed.appName,
      path,
      processId: parsed.processId
    },
    url,
    website: undefined
  });
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

  if (isLinux) {
    return readLinuxActiveWindowSync();
  }
  if (isMac) {
    return readMacActiveWindowSync();
  }
  lastError = new Error("Active window is not supported on this platform.");
  return null;
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

  if (isLinux) {
    return readLinuxActiveWindowAsync();
  }
  if (isMac) {
    return readMacActiveWindowAsync();
  }
  lastError = new Error("Active window is not supported on this platform.");
  return null;
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
