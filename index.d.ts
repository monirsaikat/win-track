export interface ActiveWindowInfo {
  appName?: string;
  title?: string;
  url?: string;
  id?: number | string;
  bounds?: {
    x: number;
    y: number;
    width: number;
    height: number;
  };
  owner?: {
    name?: string;
    path?: string;
    processId?: number;
  };
  memoryUsage?: number;
  website?: string;
}

export interface TrackingOptions {
  intervalMs?: number;
}

export function configure(options?: TrackingOptions): void;
export function startTracking(options?: TrackingOptions): void;
export function stopTracking(): void;
export function getActiveWindow(): ActiveWindowInfo | null;
export function getActiveWindowAsync(): Promise<ActiveWindowInfo | null>;
export function getLastError(): unknown;
export function onChange(
  listener: (info: ActiveWindowInfo | null) => void
): () => void;
