# 文档索引

> 最后更新：2026-06-05

## 设计准则与决策

| 文件 | 说明 |
|------|------|
| [human-decisions.md](human-decisions.md) | **权威来源**：设计准则（ARCH/CONCUR/ROBUST/INFRA/VIEW 五领域体系）+ 活跃决策 |

## 开发者文档 (developer/)

| 文档 | 说明 |
|------|------|
| [architecture/overview.md](developer/architecture/overview.md) | 框架六层架构 |
| [architecture/audio-library-design.md](developer/architecture/audio-library-design.md) | 独立音频库设计 |
| [architecture/data-flow/data-flow-design.md](developer/architecture/data-flow/data-flow-design.md) | 数据流设计（流水线 + 层依赖 DAG + 脏传播） |
| [architecture/data-flow/ds-format.md](developer/architecture/data-flow/ds-format.md) | 工程格式规范（.dsproj, .dstext, PipelineContext） |
| [architecture/data-flow/component-design.md](developer/architecture/data-flow/component-design.md) | 核心组件设计 |
| [architecture/framework/unified-app-design.md](developer/architecture/framework/unified-app-design.md) | LabelSuite + DsLabeler 统一应用设计 |
| [testing/test-design.md](developer/testing/test-design.md) | 测试分层设计 |

## 指南 (guides/)

| 文件 | 说明 |
|------|------|
| [build.md](guides/build.md) | 构建指南 + dsfw 外部集成 |
| [conventions.md](guides/conventions.md) | 编码约定与规范 |