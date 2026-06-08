# 桌面端运行说明

本目录是 Electron + React + TypeScript 桌面界面，负责调用项目根目录下的 `compressor.exe`。

## 1. 先编译 C++ 压缩程序

在项目根目录执行：

```powershell
cl /std:c++17 /utf-8 /EHsc /O2 /I src\native\include /I src\native\third_party\stb src\native\src\compressor_cli.cpp src\native\src\compress_algorithm.cpp src\native\src\directory_compress.cpp src\native\src\image_compress.cpp /Fe:compressor.exe(vs环境下运行，根目录下有.exe文件就不需要运行了)
```

确保生成：

```text
项目根目录\compressor.exe
```
## 2.配置环境变量下载electron
```
变量名：ELECTRON_MIRROR
变量值：https://npmmirror.com/mirrors/electron/
```
## 3. 构建前端

```powershell
npm run build
```

## 4. 安装前端依赖

进入桌面端目录：

```powershell
cd src\frontend
npm start
```

## 3. 开发模式运行

```powershell
npm run dev
```

构建后可运行：

```powershell
npm start
```

## 5. 功能说明

界面支持：

- 目录压缩：`dir-compress`
- 目录解压：`dir-decompress`
- 目录校验：`dir-verify`
- 灰度图统计、压缩、解压、校验
- RGB 图统计、压缩、解压、校验

所有操作最终都会通过 Electron 主进程调用项目根目录下的 `compressor.exe`。
