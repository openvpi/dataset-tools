# Bug 报告

基于代码审查发现的缺陷汇总。按严重程度分级。

---

## 中等 (P1 - 功能异常)

### BUG-009: MinLabel字典文件仅Windows复制

- **文件:** `src/apps/MinLabel/CMakeLists.txt` L39-48 (推断)
- **描述:** cpp-pinyin字典文件复制逻辑只在Windows生效, macOS/Linux运行时找不到字典.
- **影响:** macOS/Linux上G2P功能崩溃

---

## 低 (P2 - 轻微问题)

### BUG-012: GAME const_cast UB风险

- **文件:** `src/infer/game-infer/src/GameModel.cpp` L402-403
- **描述:** `const_cast<float*>(waveform.data())` 对const引用数据去const, ONNX RT若修改则为UB
- **同样存在于:** `some-infer/SomeModel.cpp` L87, `rmvpe-infer/RmvpeModel.cpp` L88
