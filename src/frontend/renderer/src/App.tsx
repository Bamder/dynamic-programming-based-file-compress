import { useMemo, useState, type ReactNode } from "react";
import type { CompressorResult, DesktopApi } from "../../electron/preload";

type Mode = "directory" | "image";
type ImageKind = "gray" | "rgb";

type FieldName =
  | "dirInput"
  | "dirArchive"
  | "dirRestore"
  | "imageInput"
  | "imageArchive"
  | "imageOutput";

const imageExtensions = {
  gray: "dpgc",
  rgb: "dprc"
};

function formatDuration(ms: number) {
  if (ms < 1000) {
    return `${ms} ms`;
  }
  return `${(ms / 1000).toFixed(2)} s`;
}

function appendExtension(filePath: string, extension: string) {
  if (!filePath) {
    return filePath;
  }
  return filePath.toLowerCase().endsWith(`.${extension}`) ? filePath : `${filePath}.${extension}`;
}

function buildCompressorArgs(command: string, paths: string[], showMetrics: boolean) {
  const supportsMetrics =
    command.endsWith("-compress") || command.endsWith("-decompress");
  if (showMetrics && supportsMetrics) {
    return [command, "--metrics", ...paths];
  }
  return [command, ...paths];
}

function getDesktopApi(): DesktopApi | undefined {
  return window.desktopApi;
}

export default function App() {
  const desktopApi = getDesktopApi();
  const [mode, setMode] = useState<Mode>("directory");
  const [imageKind, setImageKind] = useState<ImageKind>("gray");
  const [running, setRunning] = useState(false);
  const [showMetrics, setShowMetrics] = useState(true);
  const [isLogOpen, setIsLogOpen] = useState(true);
  const [lastResult, setLastResult] = useState<CompressorResult | null>(null);
  const [log, setLog] = useState("等待操作。请先确认项目根目录已有 compressor.exe。\n");
  const [fields, setFields] = useState<Record<FieldName, string>>({
    dirInput: "",
    dirArchive: "",
    dirRestore: "",
    imageInput: "",
    imageArchive: "",
    imageOutput: ""
  });

  const statusText = useMemo(() => {
    if (running) {
      return "运行中";
    }
    if (!lastResult) {
      return "就绪";
    }
    return lastResult.ok ? "成功" : "失败";
  }, [lastResult, running]);

  const statusIcon = useMemo(() => {
    if (running) {
      return "⏳";
    }
    if (!lastResult) {
      return "●";
    }
    return lastResult.ok ? "✓" : "!";
  }, [lastResult, running]);

  function updateField(name: FieldName, value: string) {
    setFields((current) => ({ ...current, [name]: value }));
  }

  function writeLog(title: string, args: string[], result: CompressorResult) {
    const commandLine = ["compressor.exe", ...args].join(" ");
    const content = [
      `\n[${title}]`,
      `命令：${commandLine}`,
      `状态：${result.ok ? "成功" : "失败"}`,
      `耗时：${formatDuration(result.durationMs)}`,
      result.stdout ? `输出：\n${result.stdout.trimEnd()}` : "输出：无",
      result.stderr ? `错误：\n${result.stderr.trimEnd()}` : "错误：无"
    ].join("\n");

    setLog((current) => `${current}${content}\n`);
    setIsLogOpen(true);
  }

  async function run(title: string, args: string[]) {
    if (!desktopApi) {
      setLog((current) => `${current}\n[${title}] Electron API 未连接，请使用 npm run dev 弹出的 Electron 桌面窗口。\n`);
      setIsLogOpen(true);
      return;
    }

    if (args.some((arg) => !arg.trim())) {
      setLog((current) => `${current}\n[${title}] 参数不完整，请先选择输入和输出路径。\n`);
      setIsLogOpen(true);
      return;
    }

    setRunning(true);
    setLastResult(null);
    try {
      const result = await desktopApi.runCompressor(args);
      setLastResult(result);
      writeLog(title, args, result);
    } finally {
      setRunning(false);
    }
  }

  async function pickDirectory(field: FieldName) {
    if (!desktopApi) {
      setLog((current) => `${current}\n[选择目录] Electron API 未连接，请使用 Electron 桌面窗口。\n`);
      setIsLogOpen(true);
      return;
    }
    const selected = await desktopApi.openDirectory();
    if (selected) {
      updateField(field, selected);
    }
  }

  async function pickFile(field: FieldName, filters?: Electron.FileFilter[]) {
    if (!desktopApi) {
      setLog((current) => `${current}\n[选择文件] Electron API 未连接，请使用 Electron 桌面窗口。\n`);
      setIsLogOpen(true);
      return;
    }
    const selected = await desktopApi.openFile(filters);
    if (selected) {
      updateField(field, selected);
    }
  }

  async function saveFile(field: FieldName, defaultName: string, filters?: Electron.FileFilter[]) {
    if (!desktopApi) {
      setLog((current) => `${current}\n[保存文件] Electron API 未连接，请使用 Electron 桌面窗口。\n`);
      setIsLogOpen(true);
      return;
    }
    const selected = await desktopApi.saveFile({ defaultPath: defaultName, filters });
    if (selected) {
      const firstExtension = filters?.[0]?.extensions?.[0];
      updateField(field, firstExtension ? appendExtension(selected, firstExtension) : selected);
    }
  }

  const imageArchiveExtension = imageExtensions[imageKind];

  return (
    <main className={`app ${isLogOpen ? "log-open" : "log-collapsed"}`}>
      <aside className="activity-bar" aria-label="主导航">
        <button className={mode === "directory" ? "activity active" : "activity"} title="目录压缩" aria-label="目录压缩" onClick={() => setMode("directory")}>📁</button>
        <button className={mode === "image" ? "activity active" : "activity"} title="图片压缩" aria-label="图片压缩" onClick={() => setMode("image")}><span className="iconfont icon-tupian"></span></button>
      </aside>

      <section className="workspace">
        <header className="title-bar">
          <div>
            <p className="eyebrow">Dynamic Programming Compressor</p>
            <h1>文件压缩工作台</h1>
          </div>
          <div className={`status ${lastResult?.ok ? "success" : lastResult ? "error" : ""}`} title="当前任务状态">
            <span className="status-dot">{statusIcon}</span>
            <div>
              <span>{statusText}</span>
              <strong>{lastResult ? formatDuration(lastResult.durationMs) : "Ready"}</strong>
            </div>
          </div>
        </header>

        <section className="content-card">
          <div className="section-heading">
            <div>
              <h2>{mode === "directory" ? "目录模式" : "PNG 图片模式"}</h2>
              <p>{mode === "directory" ? "压缩、恢复并校验完整目录结构。" : "支持灰度图与 RGB 图像的统计、压缩、恢复和校验。"}</p>
            </div>
            <div className="section-controls">
              {mode === "image" ? (
                <div className="segmented" aria-label="图片类型">
                  <button className={imageKind === "gray" ? "active" : ""} title="切换到灰度图模式" onClick={() => setImageKind("gray")}>◐ 灰度</button>
                  <button className={imageKind === "rgb" ? "active" : ""} title="切换到 RGB 图模式" onClick={() => setImageKind("rgb")}>◉ RGB</button>
                </div>
              ) : null}
              <label className="metrics-toggle" title="压缩或解压完成后在日志中输出统计信息">
                <input
                  type="checkbox"
                  checked={showMetrics}
                  onChange={(event) => setShowMetrics(event.target.checked)}
                />
                <span>输出统计信息</span>
              </label>
            </div>
          </div>

          {mode === "directory" ? (
            <div className="form-grid">
              <PathRow label="输入目录" value={fields.dirInput} onChange={(value) => updateField("dirInput", value)} onPick={() => pickDirectory("dirInput")} icon="📂" title="选择要压缩或校验的原始目录" />
              <PathRow label="压缩文件 .dpdc" value={fields.dirArchive} onChange={(value) => updateField("dirArchive", value)} onPick={() => saveFile("dirArchive", "output.dpdc", [{ name: "DPDC 压缩文件", extensions: ["dpdc"] }])} icon="💾" title="选择 .dpdc 输出文件位置" />
              <PathRow label="解压输出目录" value={fields.dirRestore} onChange={(value) => updateField("dirRestore", value)} onPick={() => pickDirectory("dirRestore")} icon="📁" title="选择解压结果输出目录" />

              <div className="actions">
                <button className="primary" disabled={running} title="将输入目录压缩为 .dpdc 文件" onClick={() => run("目录压缩", buildCompressorArgs("dir-compress", [fields.dirInput, fields.dirArchive], showMetrics))}><span className="iconfont icon-folder-zip-line"></span></button>
                <button className="soft" disabled={running} title="从 .dpdc 文件恢复目录" onClick={() => run("目录解压", buildCompressorArgs("dir-decompress", [fields.dirArchive, fields.dirRestore], showMetrics))}><span className="iconfont icon-jiemijieya"></span></button>
                <button className="soft" disabled={running} title="比较原目录与解压目录是否一致" onClick={() => run("目录校验", ["dir-verify", fields.dirInput, fields.dirRestore])}><span className="iconfont icon-xiaoyan"></span></button>
              </div>
            </div>
          ) : (
            <div className="form-grid">
              <PathRow
                label="输入 PNG"
                value={fields.imageInput}
                onChange={(value) => updateField("imageInput", value)}
                onPick={() => pickFile("imageInput", [{ name: "PNG 图片", extensions: ["png"] }])}
                icon={<span className="iconfont icon-tupian"></span>}
                title="选择待处理的 PNG 图片"
              />
        
              <PathRow label={`压缩文件 .${imageArchiveExtension}`} value={fields.imageArchive} onChange={(value) => updateField("imageArchive", value)} onPick={() => saveFile("imageArchive", `image.${imageArchiveExtension}`, [{ name: `${imageArchiveExtension.toUpperCase()} 压缩文件`, extensions: [imageArchiveExtension] }])} icon="💾" title="选择图片压缩文件输出位置" />
              <PathRow label="解压输出 PNG" value={fields.imageOutput} onChange={(value) => updateField("imageOutput", value)} onPick={() => saveFile("imageOutput", "restored.png", [{ name: "PNG 图片", extensions: ["png"] }])} icon="📤" title="选择恢复后的 PNG 输出位置" />

              <div className="actions">
                <button className="soft" disabled={running} title="查看图片统计信息" onClick={() => run("图片统计", [imageKind, fields.imageInput])}><span className="iconfont icon-tongji"></span></button>
                <button className="primary" disabled={running} title="压缩当前 PNG 图片" onClick={() => run("图片压缩", buildCompressorArgs(`${imageKind}-compress`, [fields.imageInput, fields.imageArchive], showMetrics))}><span className="iconfont icon-folder-zip-line"></span></button>
                <button className="soft" disabled={running} title="将压缩文件恢复为 PNG" onClick={() => run("图片解压", buildCompressorArgs(`${imageKind}-decompress`, [fields.imageArchive, fields.imageOutput], showMetrics))}><span className="iconfont icon-jiemijieya"></span></button>
                <button className="soft" disabled={running} title="比较原图与恢复图是否一致" onClick={() => run("图片校验", [`${imageKind}-verify`, fields.imageInput, fields.imageOutput])}><span className="iconfont icon-xiaoyan"></span></button>
              </div>
            </div>
          )}
        </section>
      </section>

      <section className="bottom-panel" aria-label="运行日志面板">
        <button className="panel-tab" title={isLogOpen ? "收起运行日志" : "展开运行日志"} onClick={() => setIsLogOpen((value) => !value)}>
          <span className={`iconfont ${isLogOpen ? "icon-zhankai" : "icon-zhankaixiangshang"}`} />
     
          <strong>运行日志</strong>
          <em>{lastResult ? `${statusText} · ${formatDuration(lastResult.durationMs)}` : "等待任务"}</em>
        </button>
        {isLogOpen ? (
          <div className="log-body">
            <div className="log-toolbar">
              <span>TERMINAL</span>
              <div>
                <button className="icon-button" title="清空日志" onClick={() => setLog("")}>🧹</button>
                <button className="icon-button" title="收起面板" onClick={() => setIsLogOpen(false)}>—</button>
              </div>
            </div>
            <pre>{log}</pre>
          </div>
        ) : null}
      </section>
    </main>
  );
}

interface PathRowProps {
  label: string;
  value: string;
  icon: ReactNode;
  title: string;
  onChange(value: string): void;
  onPick(): void;
}

function PathRow({ label, value, icon, title, onChange, onPick }: PathRowProps) {
  return (
    <label className="path-row">
      <span>{label}</span>
      <div>
        <input value={value} onChange={(event) => onChange(event.target.value)} placeholder="请选择或输入路径" title={value || label} />
        <button type="button" className="browse-button" title={title} aria-label={title} onClick={onPick}>{icon}</button>
      </div>
    </label>
  );
}
