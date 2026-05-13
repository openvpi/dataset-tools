## 设计准则 (P-01~P-19)

| # | 原则 | 约束 |
|---|---|---|
| P-01 | 模块职责单一 | 相同行为只存在一处，容器统一分发通知 |
| P-02 | 被动接口+容器通知 | 纯数据接口不加QObject，通知由容器负责 |
| P-03 | 异步一切 | >50ms操作禁止主线程同步，禁止processEvents |
| P-04 | 错误根因传播 | 错误消息追溯到根因，不得忽略后报二次错误 |
| P-05 | 异常边界隔离 | Result\<T\>传播错误，try-catch仅限第三方边界 |
| P-06 | 接口稳定 | 公共头文件即契约 |
| P-07 | 简洁可靠 | 遇错直接返回，不设计重试/回滚 |
| P-08 | 每页独立资源 | ServiceLocator仅用于全局服务，页面资源不得全局共享 |
| P-09 | 存活令牌 | QtConcurrent::run中原始指针必须持有shared_ptr\<atomic\<bool\>\>令牌 |
| P-10 | 统一路径库 | std::filesystem跨平台路径库，禁止各处自行拼接 |
| P-11 | 多线程安全 | 共享可变状态必须锁或原子操作保护 |
| P-12 | 相似模块统一 | >60%相同→合并为同一类+配置开关；30-60%→提取基类 |
| P-13 | RAII资源管理 | 禁止裸new/delete，禁止手动lock/unlock |
| P-14 | 组合优于继承 | 优先组合/委托复用功能 |
| P-15 | 依赖倒置 | 高层模块依赖抽象接口 |
| P-16 | 开闭原则 | 新增功能通过新增类实现，不修改核心逻辑 |
| P-17 | 文档模型+适配器隔离 | 内部文档模型+所有文件格式通过IFormatAdapter对接 |
| P-18 | 可测试性优先 | 接口设计可mock，构造函数注入优于ServiceLocator |
| P-19 | 配置键统一 | SettingsKey\<T\>强类型，集中定义在Keys.h |

## 待实施决策 (D-items)

### D-42：FA原生输出完整层级从属关系 [已完成]

- ✅ buildFaLayers直接替换grapheme层
- ✅ LayerDependency持久化到DsTextDocument::dependencies
- ✅ readFaInput从grapheme层读取时过滤SP/AP文本
- ✅ 层名fa_grapheme→grapheme，全库无遗留引用
- ✅ buildFaLayers()输出word.start+word.end边界
- ✅ word.end + phone[end].end绑定已建立
- ✅ 两个调用点（L653单切片、L923批量处理）均更新

### D-43：PitchLabelerPage添加工具栏+音频播放修复 [待实施]

- PitchLabelerPage创建独立QToolBar成员，包含：播放/暂停、停止、分隔符、提取F0(RMVPE)、提取MIDI(GAME)、分隔符、保存、Zoom In/Zoom Out
- 工具栏按钮使用已有action（m_extractPitchAction, m_extractMidiAction等）
- 修复onSliceSelectedImpl()：确保validatedAudioPath返回非空时无条件loadAudio
- 使用QToolButton+SVG图标，风格与PhonemeLabelerPage一致

### D-54：删除旧Widget死代码 [待实施]

删除以下6个文件（~655行）：

| 文件 | 状态 |
|------|------|
| WaveformWidget.h | 死代码，编译但不使用 |
| WaveformWidget.cpp | 死代码，编译但不使用 |
| SpectrogramWidget.h | 死代码，编译但不使用 |
| SpectrogramWidget.cpp | 死代码，编译但不使用 |
| PowerWidget.h | 死代码，编译但不使用 |
| PowerWidget.cpp | 死代码，编译但不使用 |

合计 ~836 行（旧计 ~655 行有误）。无运行时影响（无外部引用），编译时间略减。

## ADR冲突解决

| 冲突项 | 旧决策 | 新决策（以此为准） |
|--------|--------|-------------------|
| 标签区域位置 | TierEditWidget在频谱图下方 | D-15：标签区域在刻度线下方、所有子图上方 |
| 子图配置 | D-14：子图顺序可在Settings自定义 | D-30：显隐+顺序统一配置，checkbox列表+拖拽排序 |
| 贯穿线定位 | D-37：removeTierLabelArea后贯穿线未正确延伸 | D-40：移除后设置m_extraTopOffset确保贯穿线覆盖编辑区 |
| Chart边界绘制 | Chart widget不绘制边界线，仅依赖透明overlay | D-41：ChartPanelBase::paintEvent 调用 drawBoundaries() |
| Chart边界绘制(修订) | D-41: ChartPanelBase内部绘制边界线 | **本次重构**：移除ChartPanelBase::drawBoundaries()，统一由BoundaryOverlayWidget叠加层绘制，确保贯穿线不被splitter分隔线打断；所有边界线绘制统一委托给BoundaryLineRenderer |
| FA层级输出 | buildFaLayers只输出word.start边界，applyFaResult跳过grapheme | D-42：输出完整start/end边界+LayerDependency结构；applyFaResult直接替换grapheme层 |
| PitchLabel工具栏 | PitchLabelerPage仅有菜单栏，无工具栏 | D-43：添加QToolBar含F0/GAME按钮 |
