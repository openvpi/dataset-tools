# 架构与设计问题

## ARCH-001: infer-common模块被全部推理库忽略 [高]

- **问题**: dstools-infer-common提供OnnxEnv单例和统一的Session创建, 但game-infer, some-infer, rmvpe-infer, hubert-infer, FunAsr全部自建Ort::Env
- **根因**: 可能是各库独立开发后合并, 从未统一
- **后果**: 每个DLL创建独立Ort::Env实例; EP初始化代码重复5次; 修改需同步5处
- **建议**: 强制所有推理库链接并使用infer-common, 删除各自的initDirectML/initCUDA

## ARCH-002: FunAsr架构质量远低于其他模块 [高]

- **问题**: FunAsr使用C风格代码(malloc/free, raw new, 全局状态), 与项目其余部分(现代C++17, RAII, PIMPL)格格不入
- **根因**: 可能从FunASR上游直接复制, 未适配项目规范
- **后果**: 内存泄漏、线程不安全(FFTW plan)、不可维护
- **建议**: 要么重写为现代C++, 要么隔离为独立进程/subprocess调用

## ARCH-003: Widgets层直接依赖具体Audio实现 [中]

- **问题**: PlayWidget直接new AudioDecoder和AudioPlayback, 无抽象层
- **后果**: 无法mock测试, 更换音频后端需修改widget代码
- **建议**: 引入IAudioPlayer接口, PlayWidget通过DI获取

## ARCH-004: TranscriptionPipeline未使用IStepPlugin [中]

- **问题**: IStepPlugin接口已设计, 但TranscriptionPipeline硬编码TextGridToCsv→PhNumCalculator→CsvToDsConverter步骤
- **后果**: 插件系统是死代码; 添加新步骤需修改Pipeline源码
- **建议**: Pipeline消费IStepPlugin列表, 各步骤实现该接口

## ARCH-005: Theme单例和AppSettings全局函数妨碍测试 [中]

- **问题**: Theme::instance()是全局单例, fileIOProvider()是全局函数
- **后果**: 单元测试无法隔离/mock这些依赖
- **建议**: 考虑引入ServiceLocator或DI容器

## ARCH-006: DiffSingerLabeler与独立应用代码重复 [中]

- **问题**: LabelerWindow的步骤2/4/8分别嵌入MinLabelPage/PhonemeLabelerPage/PitchLabelerPage的实例, 但步骤0/1/3未集成对应的SlicerPage/LyricFAPage/HubertFAPage
- **观察**: pipeline应用中的页面与labeler的页面似乎本应统一, 但目前两套代码共存
- **建议**: 提取可复用的Page组件, labeler和pipeline共享

## ARCH-009: 错误处理缺乏统一策略 [中]

- **现状**:
  - Core: std::string &error 输出参数
  - Apps: QMessageBox弹窗
  - Infer: 某些返回bool, 某些抛异常, 某些静默失败
- **问题**: 调用者必须了解每个函数的错误约定, 容易遗漏
- **建议**: 引入统一的Result<T> 或 Expected<T, Error> 类型
