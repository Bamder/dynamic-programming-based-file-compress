import { contextBridge, ipcRenderer } from "electron";

export interface CompressorResult {
  ok: boolean;
  code: number | null;
  stdout: string;
  stderr: string;
  durationMs: number;
}

export interface DesktopApi {
  openDirectory(): Promise<string | null>;
  openFile(filters?: Electron.FileFilter[]): Promise<string | null>;
  saveFile(options?: Electron.SaveDialogOptions): Promise<string | null>;
  runCompressor(args: string[]): Promise<CompressorResult>;
}

const api: DesktopApi = {
  openDirectory: () => ipcRenderer.invoke("dialog:open-directory"),
  openFile: (filters) => ipcRenderer.invoke("dialog:open-file", filters),
  saveFile: (options) => ipcRenderer.invoke("dialog:save-file", options),
  runCompressor: (args) => ipcRenderer.invoke("compressor:run", args)
};

contextBridge.exposeInMainWorld("desktopApi", api);
