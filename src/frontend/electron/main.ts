import { app, BrowserWindow, dialog, ipcMain } from "electron";
import { spawn } from "child_process";
import path from "path";
import fs from "fs";

interface CompressorResult {
  ok: boolean;
  code: number | null;
  stdout: string;
  stderr: string;
  durationMs: number;
}

function getProjectRoot(): string {
  return path.resolve(__dirname, "../../..");
}

function getCompressorPath(): string {
  const executable = process.platform === "win32" ? "compressor.exe" : "compressor";
  return path.join(getProjectRoot(), executable);
}

function createWindow() {
  const win = new BrowserWindow({
    width: 1180,
    height: 760,
    minWidth: 980,
    minHeight: 640,
    title: "动态规划文件压缩工具",
    backgroundColor: "#0f172a",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false
    }
  });

  if (process.env.VITE_DEV_SERVER_URL) {
    win.loadURL(process.env.VITE_DEV_SERVER_URL);
  } else {
    win.loadFile(path.join(__dirname, "../dist-renderer/index.html"));
  }
}

app.whenReady().then(() => {
  createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

ipcMain.handle("dialog:open-directory", async () => {
  const result = await dialog.showOpenDialog({
    properties: ["openDirectory"]
  });
  return result.canceled ? null : result.filePaths[0];
});

ipcMain.handle("dialog:open-file", async (_event, filters?: Electron.FileFilter[]) => {
  const result = await dialog.showOpenDialog({
    properties: ["openFile"],
    filters
  });
  return result.canceled ? null : result.filePaths[0];
});

ipcMain.handle("dialog:save-file", async (_event, options?: Electron.SaveDialogOptions) => {
  const result = await dialog.showSaveDialog(options ?? {});
  return result.canceled ? null : result.filePath;
});

ipcMain.handle("compressor:run", async (_event, args: string[]): Promise<CompressorResult> => {
  const compressorPath = getCompressorPath();
  const start = Date.now();

  if (!fs.existsSync(compressorPath)) {
    return {
      ok: false,
      code: -1,
      stdout: "",
      stderr: `未找到 ${compressorPath}。请先在项目根目录编译生成 compressor.exe。`,
      durationMs: Date.now() - start
    };
  }

  return new Promise((resolve) => {
    const child = spawn(compressorPath, args, {
      cwd: getProjectRoot(),
      windowsHide: true
    });

    let stdout = "";
    let stderr = "";

    child.stdout.on("data", (data: Buffer) => {
      stdout += data.toString("utf8");
    });

    child.stderr.on("data", (data: Buffer) => {
      stderr += data.toString("utf8");
    });

    child.on("error", (error) => {
      resolve({
        ok: false,
        code: -1,
        stdout,
        stderr: stderr || error.message,
        durationMs: Date.now() - start
      });
    });

    child.on("close", (code) => {
      resolve({
        ok: code === 0,
        code,
        stdout,
        stderr,
        durationMs: Date.now() - start
      });
    });
  });
});
