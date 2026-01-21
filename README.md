# win-track

Track the active window on Windows with a sync-friendly API and native addon.

## Install

```bash
npm install win-track
```

## Usage

```js
const { getActiveWindow } = require("win-track");

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
const { getActiveWindowAsync } = require("win-track");

async function run() {
  const info = await getActiveWindowAsync();
  console.log(info);
}

run();
```

## API

- `getActiveWindow()` - Fetch the active window info.
- `getActiveWindowAsync()` - Fetch the active window once.
- `getLastError()` - Return the last error from `getActiveWindow`, if any.

## Notes

- Native Windows addon built with C++/N-API.
- URL extraction uses Windows UI Automation and supports Chrome, Edge, Firefox, and Brave.
