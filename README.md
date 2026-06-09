# 动态规划文件压缩工具（DP Compressor）

基于 DP 分段算法的文件/目录/图像压缩工具。核心为 C++ 命令行程序 `compressor.exe`；可选 Electron 桌面界面调用同一套能力。

```
项目根目录/
├── compressor.exe          ← CLI 可执行文件（需自行编译）
├── src/native/             ← C++ 压缩算法与 CLI
└── src/frontend/           ← Electron + React 桌面界面
```

---

## 一、编译 C++ 程序（CLI 与前端共用）

无论使用命令行还是桌面界面，都需要先在**项目根目录**生成 `compressor.exe`。

### 方式 A：Visual Studio（`cl`）

在「x64 Native Tools Command Prompt for VS」或已配置 `cl` 的终端中执行：

```powershell
cd P:\_Projects\file-compressor

cl /std:c++17 /utf-8 /EHsc /O2 ^
  /I src\native\include /I src\native\third_party\stb ^
  src\native\src\compressor_cli.cpp ^
  src\native\src\compress_algorithm.cpp ^
  src\native\src\directory_compress.cpp ^
  src\native\src\image_compress.cpp ^
  src\native\src\metrics_print.cpp ^
  /Fe:compressor.exe
```

### 方式 B：MinGW（`g++`）

```powershell
cd P:\_Projects\file-compressor

g++ -std=c++17 -O2 ^
  -I src/native/include -I src/native/third_party/stb ^
  src/native/src/compressor_cli.cpp ^
  src/native/src/compress_algorithm.cpp ^
  src/native/src/directory_compress.cpp ^
  src/native/src/image_compress.cpp ^
  src/native/src/metrics_print.cpp ^
  -o compressor.exe
```

编译成功后，根目录应存在：

```text
P:\_Projects\file-compressor\compressor.exe
```

> 桌面程序会从项目根目录查找 `compressor.exe`，请勿只编译到子目录而未同步到根目录。

---

## 二、命令行（CLI）使用

在项目根目录执行（路径含空格时请加引号）。

### 通用说明

| 项目          | 说明                                                      |
| ------------- | --------------------------------------------------------- |
| 子命令位置    | 第一个参数必须是子命令，例如 `dir-compress`             |
| `--metrics` | 可选；须写在子命令**之后**，压缩/解压完成后输出统计 |
| 统计命令      | `gray`、`rgb` 始终输出分析结果，无需 `--metrics`    |
| 校验命令      | `*-verify` 始终输出校验结果                             |

```powershell
# 正确
.\compressor.exe dir-compress --metrics "输入目录" "输出.dpdc"

# 错误：--metrics 不能放在子命令前面
.\compressor.exe --metrics dir-compress "输入目录" "输出.dpdc"
```

### 目录

| 命令                                                  | 说明             |
| ----------------------------------------------------- | ---------------- |
| `dir-compress [--metrics] <输入目录> <输出.dpdc>`   | 压缩目录         |
| `dir-decompress [--metrics] <输入.dpdc> <输出目录>` | 解压目录         |
| `dir-verify <原始目录> <还原目录>`                  | 校验往返是否一致 |

```powershell
.\compressor.exe dir-compress --metrics ".\data\my_dir" ".\out\archive.dpdc"
.\compressor.exe dir-decompress --metrics ".\out\archive.dpdc" ".\out\restored"
.\compressor.exe dir-verify ".\data\my_dir" ".\out\restored"
```

### 灰度 PNG

| 命令                                                   | 说明                 |
| ------------------------------------------------------ | -------------------- |
| `gray <输入.png>`                                    | 仅分析，输出 DP 统计 |
| `gray-compress [--metrics] <输入.png> <输出.dpgc>`   | 压缩                 |
| `gray-decompress [--metrics] <输入.dpgc> <输出.png>` | 解压                 |
| `gray-verify <原始.png> <还原.png>`                  | 校验                 |

```powershell
.\compressor.exe gray ".\image.png"
.\compressor.exe gray-compress --metrics ".\image.png" ".\image.dpgc"
.\compressor.exe gray-decompress --metrics ".\image.dpgc" ".\restored.png"
.\compressor.exe gray-verify ".\image.png" ".\restored.png"
```

### RGB PNG

| 命令                                                  | 说明   |
| ----------------------------------------------------- | ------ |
| `rgb <输入.png>`                                    | 仅分析 |
| `rgb-compress [--metrics] <输入.png> <输出.dprc>`   | 压缩   |
| `rgb-decompress [--metrics] <输入.dprc> <输出.png>` | 解压   |
| `rgb-verify <原始.png> <还原.png>`                  | 校验   |

```powershell
.\compressor.exe rgb-compress --metrics ".\photo.png" ".\photo.dprc"
```

### 查看完整帮助

```powershell
.\compressor.exe
```

---

## 三、桌面界面（前端）使用

前端位于 `src/frontend`，通过 Electron 调用根目录的 `compressor.exe`。**必须在 Electron 窗口中操作**，不要用浏览器打开 Vite 地址。

### 1. 安装依赖

```powershell
cd P:\_Projects\file-compressor\src\frontend
npm install
```

项目已包含 `src/frontend/.npmrc`，Electron 下载默认走国内镜像。若仍失败，可在当前终端临时设置：

```powershell
$env:ELECTRON_MIRROR = "https://npmmirror.com/mirrors/electron/"
npm install
```

### 2. 开发模式（日常推荐）

```powershell
cd P:\_Projects\file-compressor\src\frontend
npm run dev
```

1. 等待终端出现 Vite 就绪，并弹出标题为 **「动态规划文件压缩工具」** 的 Electron 窗口
2. 在**该窗口**中选择路径、执行压缩/解压/校验
3. 底部「运行日志」可查看命令与输出

> **不要**在 Chrome/Edge 中访问 `http://127.0.0.1:5173`。浏览器里没有 `desktopApi`，会出现「Electron API 未连接」。

界面提供：

- **目录模式**：压缩、解压、校验
- **图片模式**：灰度 / RGB 切换；统计、压缩、解压、校验
- **输出统计信息**：勾选后，压缩/解压命令会自动附加 `--metrics`

### 3. 生产模式

```powershell
cd P:\_Projects\file-compressor\src\frontend
npm run build
npm start
```

### 常见问题

| 现象                                | 处理                                                                                        |
| ----------------------------------- | ------------------------------------------------------------------------------------------- |
| Electron API 未连接                 | 使用 `npm run dev` 弹出的 Electron 窗口，不要用浏览器                                     |
| 未找到 compressor.exe               | 在项目根目录重新编译，确认存在 `compressor.exe`                                           |
| Downloading Electron / fetch failed | 检查网络；设置 `$env:ELECTRON_MIRROR` 后删除 `node_modules\electron` 再 `npm install` |
| 压缩无统计输出                      | CLI 需加 `--metrics`；界面需勾选「输出统计信息」                                          |

---

## 四、自动化测试脚本

在项目根目录运行：

```powershell
# 目录往返
g++ -std=c++17 -O2 -I src/native/include tests/directory_roundtrip_test.cpp src/native/src/compress_algorithm.cpp src/native/src/directory_compress.cpp -o directory_roundtrip_test.exe
.\directory_roundtrip_test.exe

# 图像往返
g++ -std=c++17 -O2 -I src/native/include -I src/native/third_party/stb tests/image_roundtrip_test.cpp src/native/src/compress_algorithm.cpp src/native/src/image_compress.cpp -o image_roundtrip_test.exe
.\image_roundtrip_test.exe
```

---

## 五、文件格式

| 扩展名    | 内容         |
| --------- | ------------ |
| `.dpdc` | 目录压缩包   |
| `.dpgc` | 灰度图压缩包 |
| `.dprc` | RGB 图压缩包 |
