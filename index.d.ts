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

export function getActiveWindow(): ActiveWindowInfo | null;
export function getActiveWindowAsync(): Promise<ActiveWindowInfo | null>;
export function getLastError(): unknown;
