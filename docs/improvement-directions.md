# DiffSinger Dataset Tools 改进方向

> 更新时间：2026-05-01 | 详细任务见 [refactoring-roadmap.md](refactoring-roadmap.md)

---

## P2 — 有实际价值

| 方向 | 路线图 | 状态 |
|------|--------|------|
| ServiceLocator void* → std::any 类型安全 | A.1 | 📋 待执行 |
| 服务接口一致性修复 (IAsrService 补齐虚方法 + IInferenceEngine::load 纯虚化) | A.2 | 📋 待执行 |
| CrashHandler 所有 app 启用 | A.3 | 📋 待执行 |
| 补齐领域模块单元测试 | B.1 | ⏳ 4 个模块待测 |
| 框架模块独立编译 CI 验证 | D.1 | ⏳ 待创建 workflow |
| 升级 C++20 | E.1 | 📋 待执行 |

## P3 — 按需拾取

| 方向 | 路线图 | 说明 |
|------|--------|------|
| TODO/FIXME 清理 | B.2 | 5 处，随手修 |
| 文件操作错误处理 | B.3 | 补 else 分支 |
| 大文件拆分 | C.1 | PitchLabelerPage (781行), PhonemeLabelerPage (630行) |
| 魔法数字常量化 | C.2 | `4096` 3 处重复 |
| Doxygen CI | D.2 | 头文件注释已就绪 |
| 跨平台包分发 | D.3 | ZIP/DMG/AppImage |
| 示例项目 | F.1 | 有外部用户需求时做 |
| Undo/Redo 迁移 | F.2 | 重构相关 app 时顺便做 |
