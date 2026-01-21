"use strict";

const { getActiveWindow, onChange } = require("./index");

console.log("Starting win-trace test. Press Ctrl+C to stop.");

const stop = onChange((info) => {
  console.log("change:", {
    appName: info?.appName,
    title: info?.title,
    url: info?.url,
    id: info?.id,
    bounds: info?.bounds,
    owner: info?.owner,
    memoryUsage: info?.memoryUsage,
    website: info?.website
  });
});

setInterval(() => {
  const info = getActiveWindow();
  console.log("poll:", {
    appName: info?.appName,
    title: info?.title,
    url: info?.url,
    id: info?.id,
    bounds: info?.bounds,
    owner: info?.owner,
    memoryUsage: info?.memoryUsage,
    website: info?.website
  });
}, 2000);

process.on("SIGINT", () => {
  stop();
  process.exit(0);
});
