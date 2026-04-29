# 构建系统与CI问题

## CI-001: 无macOS/Linux CI [P0]

- 文件: `.github/workflows/` (仅windows-rel-build.yml)
- 问题: README声称支持macOS和Linux, 但无CI验证
- 影响: 跨平台兼容性无保证, 可能随时regression
- 修复: 添加macos.yml和linux.yml workflows

## BUILD-005: 5个infer库CMake模板重复 [P2]

- 位置: audio-util, game-infer, some-infer, rmvpe-infer, hubert-infer CMakeLists.txt
- 问题: ~100行相同的MSVC flags, output dirs, ORT setup, install/export 模板
- 修复: 抽取cmake/infer-common.cmake函数
