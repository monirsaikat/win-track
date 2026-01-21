# win-trace

Track the active window on Windows with a sync-friendly API and native addon.

## Install

```bash
npm install win-trace
```

## Usage

```js
const { getActiveWindow } = require("win-trace");

const info = getActiveWindow();
console.log({
  appName: info?.appName,
  title: info?.title,
  url: info?.url,
  id: info?.id,
  bounds: info?.bounds,
  owner: info?.owner,
  memoryUsage: info?.memoryUsage,
  website: info?.website
});
```

For a one-off async fetch:

```js
const { getActiveWindowAsync } = require("win-trace");

async function run() {
  const info = await getActiveWindowAsync();
  console.log(info);
}

run();
```

## API

- `configure({ intervalMs })` - Set the polling interval in milliseconds.
- `startTracking({ intervalMs })` - Start background polling.
- `stopTracking()` - Stop background polling.
- `getActiveWindow()` - Get the last cached active window info.
- `getActiveWindowAsync()` - Fetch the active window once.
- `getLastError()` - Return the last polling error, if any.
- `onChange(listener)` - Subscribe to changes and get an unsubscribe function.

## Notes

- Native Windows addon built with C++/N-API.
- URL extraction uses Windows UI Automation and supports Chrome, Edge, Firefox, and Brave.
# win-track
