# win-track

> Get the **active window** — and the **current browser tab URL** — in **Node.js** on **Windows, macOS, and Linux**.

[![npm version](https://img.shields.io/npm/v/win-track.svg)](https://www.npmjs.com/package/win-track)
[![npm downloads](https://img.shields.io/npm/dm/win-track.svg)](https://www.npmjs.com/package/win-track)
[![platforms](https://img.shields.io/badge/platform-windows%20%7C%20macos%20%7C%20linux-blue.svg)](#-supported-browsers-url-capture)
[![Node-API](https://img.shields.io/badge/Node--API-ABI%20stable-brightgreen.svg)](#%EF%B8%8F-how-it-works)
[![license](https://img.shields.io/npm/l/win-track.svg)](./LICENSE)

**win-track** is a fast, lightweight native module that tells you which window the user is focused on *right now* — the app name, window title, process, bounds, memory — and, **uniquely, the URL of the active browser tab**, with no browser extension required on Windows or macOS.

It's built on **N-API (Node-API)**, so a single native build runs across Node *and* Electron versions without recompiling — perfect for **time-tracking**, **productivity / activity-monitoring**, **screen-time**, and **RPA / automation** tools.

## ✨ Features

- 🪟 **Active / foreground window** — app name, window title, id, bounds, owner (exe path + PID) and memory usage
- 🌐 **Browser URL detection** — the current tab's URL **and** domain, no extension needed on Windows/macOS
- ⚡ **Sync *and* async** APIs — `getActiveWindow()` and `getActiveWindowAsync()`
- 🖥️ **Cross-platform** — Windows, macOS and Linux
- 🧩 **N-API / ABI-stable** — one native build works across Node *and* Electron versions (no `electron-rebuild` dance)
- 🪶 **Tiny** — no heavyweight runtime dependencies

## 🌐 Supported browsers (URL capture)

| Browser | Windows | macOS |
| --- | :---: | :---: |
| Google Chrome / Chromium | ✅ | ✅ |
| Microsoft Edge | ✅ | ✅ |
| Brave | ✅ | ✅ |
| Opera / Opera GX | ✅ | ⚠️&nbsp;\* |
| Vivaldi | ✅ | ✅ |
| Mozilla Firefox | ✅ | ✅&nbsp;\*\* |
| Safari | — | ✅ |

\* Opera URL capture on macOS depends on the build's AppleScript support.
\*\* Firefox on macOS uses an accessibility / clipboard fallback — grant Accessibility permission.

> On **Linux**, win-track returns active-window metadata (app, title, bounds, PID) via `xprop`/`xwininfo`. Browser URLs aren't reliably available from the desktop session alone on Linux.

## 📦 Install

```bash
npm install win-track
```

> On **Windows** the native addon compiles on install, so you'll need the standard build toolchain (Visual Studio Build Tools + Python). **macOS** and **Linux** use built-in system tools (`osascript`, `xprop`) and require no compilation.

## 🚀 Quick start

```js
const { getActiveWindow } = require("win-track");

const info = getActiveWindow();
console.log(info);
// {
//   appName: 'Google Chrome',
//   title: 'win-track - npm',
//   url: 'https://www.npmjs.com/package/win-track',
//   website: 'www.npmjs.com',
//   id: '1184666',
//   bounds: { x: 0, y: 0, width: 1920, height: 1080 },
//   owner: { name: 'chrome.exe', path: 'C:\\...\\chrome.exe', processId: 12345 },
//   memoryUsage: 284512256
// }
```

Async (one-off fetch):

```js
const { getActiveWindowAsync } = require("win-track");

const info = await getActiveWindowAsync();
console.log(info?.appName, "→", info?.url);
```

Poll the focused window every second:

```js
const { getActiveWindow } = require("win-track");

setInterval(() => {
  const w = getActiveWindow();
  if (w) console.log(w.appName, "→", w.url ?? w.title);
}, 1000);
```

## 📘 API

### `getActiveWindow(): ActiveWindowInfo | null`
Returns the currently focused window **synchronously**, or `null` if there isn't one.

### `getActiveWindowAsync(): Promise<ActiveWindowInfo | null>`
The same result, returned as a promise.

### `getLastError(): unknown`
The last error captured internally (e.g. a failed platform command), or `null`.

### `ActiveWindowInfo`

```ts
interface ActiveWindowInfo {
  appName?: string;   // friendly app name, e.g. "Google Chrome"
  title?: string;     // window title (cleaned of " - <Browser>" suffixes)
  url?: string;       // active browser tab URL (browsers only)
  website?: string;   // hostname derived from url, e.g. "github.com"
  id?: number | string;
  bounds?: { x: number; y: number; width: number; height: number };
  owner?: { name?: string; path?: string; processId?: number };
  memoryUsage?: number; // working set, in bytes
}
```

## ⚙️ How it works

- **Windows** — a C++ / **N-API** addon reads the foreground window via the Win32 API and extracts the browser URL using **UI Automation** (the address bar).
- **macOS** — uses **AppleScript** (`osascript`) for the frontmost app/window and the active tab's URL.
- **Linux** — uses **`xprop`** and **`xwininfo`** for active-window metadata (X11).

Because the native addon is **N-API**, the compiled binary is **ABI-stable**: it loads in any modern Node.js *and* Electron version without rebuilding.

## 💡 Use cases

Time tracking · productivity & activity monitoring · screen-time analytics · focus / digital-wellbeing apps · automated UI testing & RPA · desktop-app usage telemetry.

## ❓ FAQ

**How do I get the URL of the active browser tab in Node.js?**
Call `getActiveWindow()` (or `await getActiveWindowAsync()`). When a supported browser is focused, the result's `url` and `website` fields hold the current tab's address — no browser extension needed on Windows or macOS.

**Does it work with Electron?**
Yes. win-track is **N-API / ABI-stable**, so the same native binary loads across Electron and Node versions — no `electron-rebuild` step required.

**Is it synchronous or asynchronous?**
Both — `getActiveWindow()` is synchronous and `getActiveWindowAsync()` returns a promise.

**Which browsers can it read URLs from?**
Chrome / Chromium, Edge, Brave, Opera, Opera GX, Vivaldi and Firefox (plus Safari on macOS). See the [support table](#-supported-browsers-url-capture).

## 📄 License

MIT © [monirsaikat](https://github.com/monirsaikat)
