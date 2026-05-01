# 本地编译指南 (Windows)

> 基于 2026-05-01 全量编译验证，可直接使用。

## 环境信息

| 组件 | 路径 | 版本 |
|------|------|------|
| Visual Studio | `D:\Programs\vs2026` | MSVC 19.50.35727 (v14.50) |
| MSVC 工具链 | `D:\Programs\vs2026\VC\Tools\MSVC\14.50.35717` | — |
| CLion | `D:\Program Files\CLion\bin` | 内置 CMake 4.2 + Ninja |
| Qt | `D:\Programs\qt5\6.10.0\msvc2022_64` | 6.10.0 |
| vcpkg | 项目内 `vcpkg/` 子目录 | — |
| CMake | `D:\Program Files\CLion\bin\cmake\win\x64\bin\cmake.exe` | 4.2 |
| Ninja | `D:\Program Files\CLion\bin\ninja\win\x64\ninja.exe` | — |
| 项目目录 | `D:\projects\dataset-tools` | — |
| 构建输出 | `cmake-build-release/` (CLion) 或 `build/` (命令行) | — |

### 重要：Qt 版本锁定

vcpkg 依赖（qwindowkit、qbreakpad）编译时绑定了 Qt 6.10.0。**必须使用 6.10.0**，不能用 6.10.2。如果需要切换 Qt 版本，必须重新安装 vcpkg 依赖。

---

## 一次性环境准备

### 1. 克隆仓库 + vcpkg

```bat
git clone https://github.com/openvpi/dataset-tools.git
cd dataset-tools
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
cd ..
```

### 2. 安装 vcpkg 依赖

```bat
set QT_DIR=D:\Programs\qt5\6.10.0\msvc2022_64
set Qt6_DIR=%QT_DIR%
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR

vcpkg\vcpkg install ^
    --x-manifest-root=scripts/vcpkg-manifest ^
    --x-install-root=vcpkg/installed ^
    --triplet=x64-windows
```

### 3. 下载 ONNX Runtime (DirectML)

```bat
cd src\infer
cmake -Dep=dml -P ..\..\cmake\setup-onnxruntime.cmake
cd ..\..
```

---

## 编译脚本

项目根目录提供 `scripts/build-release.bat` 和 `scripts/build-debug.bat`，双击或命令行运行即可。

### 手动编译（VS Developer Command Prompt）

打开 **VS 2026 x64 Native Tools Command Prompt**，进入项目目录：

```bat
cmake -B build -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DCMAKE_PREFIX_PATH=D:\Programs\qt5\6.10.0\msvc2022_64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DONNXRUNTIME_ENABLE_DML=ON

cmake --build build -j 10
```

### 手动编译（普通 CMD / PowerShell，无需提前开 Developer Prompt）

```bat
call "D:\Programs\vs2026\VC\Auxiliary\Build\vcvarsall.bat" x64

set QT_DIR=D:\Programs\qt5\6.10.0\msvc2022_64
set Qt6_DIR=%QT_DIR%
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR

cmake -B cmake-build-release -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DCMAKE_PREFIX_PATH=%QT_DIR% ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DONNXRUNTIME_ENABLE_DML=ON ^
    -DCMAKE_MAKE_PROGRAM="D:\Program Files\CLion\bin\ninja\win\x64\ninja.exe"

cmake --build cmake-build-release -j 10
```

### CLion 配置

1. **Settings → Build → Toolchains**: Visual Studio，路径 `D:\Programs\vs2026`，Architecture `amd64`
2. **Settings → Build → CMake**: Profile CMake options：

```
-DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
-DCMAKE_PREFIX_PATH=D:\Programs\qt5\6.10.0\msvc2022_64
-DONNXRUNTIME_ENABLE_DML=ON
```

3. **Environment**: 添加 `QT_DIR=D:\Programs\qt5\6.10.0\msvc2022_64;Qt6_DIR=D:\Programs\qt5\6.10.0\msvc2022_64\lib\cmake\Qt6;VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR`
4. Generator: Ninja, Build type: Release 或 Debug

---

## 安装部署

```bat
cmake -B build -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DCMAKE_PREFIX_PATH=D:\Programs\qt5\6.10.0\msvc2022_64 ^
    -DCMAKE_INSTALL_PREFIX=install ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DONNXRUNTIME_ENABLE_DML=ON

cmake --build build --target install
```

安装过程会调用 `windeployqt` 打包 Qt 运行时和所有 DLL 到 `install/` 目录。

---

## 运行测试

```bat
cd build
ctest --output-on-failure
```

---

## 构建产物

| 类型 | 文件 |
|------|------|
| 应用 (6) | DatasetPipeline.exe, MinLabel.exe, PhonemeLabeler.exe, PitchLabeler.exe, GameInfer.exe, DiffSingerLabeler.exe |
| 工具 (2) | dstools-cli.exe, WidgetGallery.exe |
| 测试 (13+) | TestGame.exe, TestRmvpe.exe, TestAudioUtil.exe, TestResult.exe, TestJsonHelper.exe, TestAppSettings.exe, TestServiceLocator.exe, TestAsyncTask.exe, TestLocalFileIOProvider.exe, TestModelManager.exe, TestModelDownloader.exe, TestAppShellIntegration.exe, TestIDocument.exe, TestIFileIOProviderMock.exe, TestStringUtils.exe, TestMinLabelService.exe, dsfw-widgets-test.exe |
| 动态库 (6) | dsfw-widgets.dll, dstools-widgets.dll, audio-util.dll, game-infer.dll, rmvpe-infer.dll, hubert-infer.dll |
| Shell (1) | TestShell.exe |

所有二进制输出到 `build/bin/`，库文件输出到 `build/lib/`。

---

## CMake 选项速查

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `ONNXRUNTIME_ENABLE_DML` | ON (Windows) | DirectML GPU 加速 |
| `ONNXRUNTIME_ENABLE_CUDA` | OFF | CUDA GPU 加速 |
| `BUILD_TESTS` | ON | 构建测试目标 |
| `AUDIO_UTIL_BUILD_TESTS` | ON | TestAudioUtil |
| `GAME_INFER_BUILD_TESTS` | ON | TestGame |
| `RMVPE_INFER_BUILD_TESTS` | ON | TestRmvpe |

---

## 常见问题

### LNK1181: 无法打开输入文件 "kernel32.lib"

未正确加载 MSVC 环境。确保先执行 `vcvarsall.bat x64`，或使用 VS Developer Command Prompt。

### MOC 报 "Undefined interface"

AUTOMOC 的 include 路径缺少 `dsfw-ui-core`。对应 CMakeLists.txt 需要显式添加 `dsfw-ui-core` 到 `target_link_libraries`。

### Qt 版本不匹配 (moc 报 "cannot be used with the include files from this version of Qt")

vcpkg 安装的 qwindowkit/qbreakpad 绑定了特定 Qt 版本。确保 `QT_DIR` 和 `CMAKE_PREFIX_PATH` 指向与 vcpkg 编译时相同的 Qt 版本 (当前为 6.10.0)。

### vcpkg 包安装失败

部分包 (qwindowkit, qbreakpad) 需要 Qt 路径才能编译。确认 `QT_DIR` 和 `VCPKG_KEEP_ENV_VARS` 已设置。

### ONNX Runtime 下载失败

`setup-onnxruntime.cmake` 会自动检测系统代理。手动下载从 [NuGet](https://www.nuget.org/packages/Microsoft.ML.OnnxRuntime.DirectML) 获取 DML 版本，解压到 `src/infer/onnxruntime/`。
