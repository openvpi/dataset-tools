# 文档索引

> 最后更新：2026-06-01

## 设计准则与决策

| 文件 | 说明 |
|------|------|
| [human-decisions.md](human-decisions.md) | **权威来源**：设计准则（ARCH/CONCUR/ROBUST/INFRA/VIEW 五领域体系）+ 活跃决策 |

## 开发者文档 (developer/)

| 文档 | 说明 |
|------|------|
| [architecture/overview.md](developer/architecture/overview.md) | 框架六层架构 |
| [architecture/framework/unified-app-design.md](developer/architecture/framework/unified-app-design.md) | LabelSuite + DsLabeler 统一应用设计 |
| [architecture/data-flow/ds-format.md](developer/architecture/data-flow/ds-format.md) | DS 数据格式规范（.dsproj, .dstext, PipelineContext） |
| [architecture/data-flow/pipeline.md](developer/architecture/data-flow/pipeline.md) | 任务处理器架构与流水线定义 |
| [architecture/framework/dirty-mechanism.md](developer/architecture/framework/dirty-mechanism.md) | 标脏机制设计（层依赖 DAG + 传播规则） |
| [architecture/data-flow/component-design.md](developer/architecture/data-flow/component-design.md) | 组件设计 |
| [architecture/refactoring-plan-v1.md](developer/architecture/refactoring-plan-v1.md) | 大型重构方案 v1（统一底层 + 清偿技术债） |
| [testing/test-design.md](developer/testing/test-design.md) | 测试分层设计 |
| [magic-numbers-audit.md](developer/magic-numbers-audit.md) | 魔法数字审计报告 |

## 指南 (guides/)

| 文件 | 说明 |
|------|------|
| [build.md](guides/build.md) | 构建指南 |
| [conventions.md](guides/conventions.md) | 编码约定与规范 |
| [framework-getting-started.md](guides/framework-getting-started.md) | dsfw 框架入门 |