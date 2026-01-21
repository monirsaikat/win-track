"use strict";

const { getActiveWindow } = require("./index");

console.log("Starting win-track test. Press Ctrl+C to stop.");

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
  process.exit(0);
});
