# 构建与 CI 验证测试

---

## 1. 本地构建验证

### BD-001: CMake 配置

```bash
cmake -B build -G Ninja \
    -DCMAKE_PREFIX_PATH=$QT_DIR \
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

# 预期: 退出码 0, 无 CMake Error
```

**验证项**:
- [ ] Qt6 找到 (Core, Widgets, Svg)
- [ ] vcpkg 依赖解析成功
- [ ] ONNX Runtime 路径正确
- [ ] 所有 CMakeLists.txt 解析无错误

> **注意**: 顶层 `BUILD_TESTS` 选项仅控制 `src/tests/` 子目录（当前为空占位）。
> 各库的测试由独立选项控制：
> - `AUDIO_UTIL_BUILD_TESTS` (默认 ON)
> - `GAME_INFER_BUILD_TESTS` (默认 ON)
> - `SOME_INFER_BUILD_TESTS` (默认 ON)
> - `RMVPE_INFER_BUILD_TESTS` (默认 ON)
>
> 要禁用所有库测试，需同时设置以上选项为 OFF。

### BD-002: 全量编译

```bash
cmake --build build --target all 2>&1 | tee build.log

# 统计
grep -c "error" build.log    # 应为 0
grep -c "warning" build.log  # 记录基线
```

### BD-003: 产出完整性

```bash
# 应用 (6个)
for exe in MinLabel SlurCutter AudioSlicer LyricFA HubertFA GameInfer; do
    test -f build/bin/$exe.exe || echo "MISSING: $exe"
done

# 测试 (4个)
for exe in TestAudioUtil TestGame TestSome TestRmvpe; do
    test -f build/bin/$exe.exe || echo "MISSING: $exe"
done

# 共享库 (4个)
for lib in audio-util game-infer some-infer rmvpe-infer; do
    test -f build/bin/$lib.dll || echo "MISSING: $lib"
done
```

### BD-004: 禁用库测试

```bash
cmake -B build_notest -G Ninja \
    -DAUDIO_UTIL_BUILD_TESTS=OFF \
    -DGAME_INFER_BUILD_TESTS=OFF \
    -DSOME_INFER_BUILD_TESTS=OFF \
    -DRMVPE_INFER_BUILD_TESTS=OFF \
    ...
cmake --build build_notest

# 验证: Test*.exe 不应存在
ls build_notest/bin/Test*.exe 2>/dev/null && echo "FAIL: tests built with *_BUILD_TESTS=OFF"
```

> **注意**: 顶层 `BUILD_TESTS=OFF` 仅禁用空的 `src/tests/` 目录，不影响库测试。
> CI 中 `-DBUILD_TESTS=FALSE` 实际上不会禁用库测试目标。

### BD-005: DML=OFF

```bash
cmake -B build_nodml -G Ninja -DONNXRUNTIME_ENABLE_DML=OFF ...
cmake --build build_nodml

# 验证: 不链接 DirectML.dll
dumpbin /DEPENDENTS build_nodml/bin/GameInfer.exe | grep -i directml && echo "FAIL"
```

---

## 2. CI Workflow 验证

当前 CI: `.github/workflows/windows-rel-build.yml`

### CI-001: 触发条件

| 场景 | 预期 |
|---|---|
| Push 任意 tag (如 `v1.0.0`、`test-build` 等) | 触发构建 |
| Push branch `main` | 不触发 |
| Pull request | 不触发 |

> **注意**: CI 配置 `on.push.tags: ["*"]` 对任意 tag 都会触发，不限于 `v*` 格式。

### CI-002: 构建产出

验证 artifact `dataset-tools-msvc2022-x64-*.zip` 包含:
- [ ] 所有 6 个应用 EXE
- [ ] Qt6 DLL (windeployqt 输出)
- [ ] ONNX Runtime DLL
- [ ] SDL2.dll, sndfile.dll 等依赖

> **注意**: CI 设置了 `-DBUILD_TESTS=FALSE`，但由于各库使用独立的 `*_BUILD_TESTS`
> 选项（默认 ON），测试可执行文件仍会被编译并包含在产出中。

### CI-003: Release 创建

Tag push 后:
- [ ] GitHub Release 自动创建
- [ ] Artifact 上传到 Release assets

---

## 3. 建议 CI 改进

### 添加 PR 构建检查

```yaml
# .github/workflows/pr-check.yml (建议新增)
on:
  pull_request:
    branches: [main]

jobs:
  build:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      # ... 与现有 workflow 相同的构建步骤 ...

      # 新增: 运行 smoke test
      - name: Run smoke tests
        run: |
          ./build/bin/TestAudioUtil.exe test-data/audio/test_mono_44100.wav $env:TEMP/out.wav
        continue-on-error: true
```

### 添加构建缓存

```yaml
      - name: Cache vcpkg
        uses: actions/cache@v4
        with:
          path: vcpkg/installed
          key: vcpkg-${{ hashFiles('scripts/vcpkg-manifest/vcpkg.json') }}
```
