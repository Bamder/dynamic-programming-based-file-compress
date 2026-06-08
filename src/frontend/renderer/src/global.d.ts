import type { DesktopApi } from "../../electron/preload";

export {};

declare global {
  interface Window {
    desktopApi: DesktopApi;
  }
}
