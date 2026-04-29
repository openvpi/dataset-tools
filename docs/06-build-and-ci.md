# 构建系统与CI问题

## CI-001: 无macOS/Linux CI [P0]

- 文件: `.github/workflows/` (仅windows-rel-build.yml)
- 问题: README声称支持macOS和Linux, 但无CI验证
- 影响: 跨平台兼容性无保证, 可能随时regression
- 修复: 添加macos.yml和linux.yml workflows

## CI-002: 测试从不在CI中运行 [P0]

- 文件: `.github/workflows/windows-rel-build.yml` L79
- 问题: `-DBUILD_TESTS:BOOL=FALSE`, 且无test step
- 影响: 回归无法自动检测
- 修复: BUILD_TESTS=ON + ctest运行步骤

## CI-003: CI仅tag触发 [P1]

- 文件: `.github/workflows/windows-rel-build.yml` L4-6
- 问题: 仅在push tag时触发, PR和branch push无CI
- 影响: 代码可在无任何验证的情况下merge
- 修复: 添加PR触发条件

## BUILD-002: Linux无install/deploy逻辑 [P1]

- 文件: `src/CMakeLists.txt` L50-115
- 问题: WIN32和APPLE分支后无else(), Linux install为空操作
- 影响: `cmake --install` 在Linux上不复制任何文件
- 修复: 添加Linux安装逻辑 (file COPY + 可选linuxdeploy)

## BUILD-004: 下载无hash校验 [P1]

- 文件: `cmake/setup-onnxruntime.cmake` L105
- 问题: `EXPECTED_HASH` 被注释掉
- 影响: 供应链攻击风险
- 修复: 恢复hash校验

## BUILD-005: 5个infer库CMake模板重复 [P2]

- 位置: audio-util, game-infer, some-infer, rmvpe-infer, hubert-infer CMakeLists.txt
- 问题: ~100行相同的MSVC flags, output dirs, ORT setup, install/export 模板
- 修复: 抽取cmake/infer-common.cmake函数

## BUILD-006: Windows install复制所有dll/exe [P2]

- 文件: `src/CMakeLists.txt` L54
- 问题: `file(GLOB _files "*.dll" "*.exe")` 不区分, 包含test可执行文件
- 修复: 只复制DeployedTargets列表中的target

## BUILD-007: NuGet DML包硬编码x64 [P2]

- 文件: `cmake/setup-onnxruntime.cmake` L153
- 问题: `runtimes/win-x64/native` 不支持ARM64 Windows
- 修复: 根据CMAKE_SYSTEM_PROCESSOR选择

## BUILD-010: vcpkg.json中ffmpeg-fake [P3]

- 文件: `scripts/vcpkg-manifest/vcpkg.json`
- 问题: 依赖名为 "ffmpeg-fake", 是自定义overlay port, 无文档说明
- 修复: 添加注释或README说明

## BUILD-011: 无Linux ARM64支持 [P2]

- 文件: `cmake/setup-onnxruntime.cmake` L84
- 问题: Linux非x86_64直接FATAL_ERROR
- 影响: Jetson等ARM64 Linux设备无法使用
- 修复: 添加aarch64下载URL或允许用户指定ORT路径
