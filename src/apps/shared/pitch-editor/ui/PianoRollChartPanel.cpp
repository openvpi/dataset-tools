#include "PianoRollChartPanel.h"

#include "ChartConfigRegistry.h"

#include <QVBoxLayout>
#include <dsfw/AppSettings.h>
#include "PianoRollRenderer.h"

namespace dstools {
namespace pitchlabeler {
namespace ui {

PianoRollChartPanel::PianoRollChartPanel(dsfw::widgets::ViewportController *vc,
                                         QWidget *parent)
    : chart::ChartPanelBase("pianoroll", vc, parent)
    , m_pianoRoll(new PianoRollView(this))
{
    loadConfigParams();

    // Set up piano roll
    m_pianoRoll->setViewportController(vc);
    m_pianoRoll->setContentLeftMargin(m_configPianoWidth);
    m_pianoRoll->setLayoutConfig(m_configPianoWidth, m_configScrollBarSize,
                                 m_configMinMidi, m_configMaxMidi,
                                 m_configModSensitivity);
    
    // Connect signals from piano roll to our signals
    connect(m_pianoRoll, &PianoRollView::noteSelected,
            this, &PianoRollChartPanel::noteSelected);
    connect(m_pianoRoll, &PianoRollView::toolModeChanged,
            this, &PianoRollChartPanel::toolModeChanged);
    connect(m_pianoRoll, &PianoRollView::fileEdited,
            this, &PianoRollChartPanel::fileEdited);
    connect(m_pianoRoll, &PianoRollView::positionClicked,
            this, &PianoRollChartPanel::positionClicked);
    connect(m_pianoRoll, &PianoRollView::rulerClicked,
            this, &PianoRollChartPanel::rulerClicked);
    connect(m_pianoRoll, &PianoRollView::noteGlideChanged,
            this, &PianoRollChartPanel::noteGlideChanged);
    connect(m_pianoRoll, &PianoRollView::noteSlurToggled,
            this, &PianoRollChartPanel::noteSlurToggled);
    connect(m_pianoRoll, &PianoRollView::noteRestToggled,
            this, &PianoRollChartPanel::noteRestToggled);
    connect(m_pianoRoll, &PianoRollView::noteMergeLeft,
            this, &PianoRollChartPanel::noteMergeLeft);
    connect(m_pianoRoll, &PianoRollView::noteDeleteRequested,
            this, &PianoRollChartPanel::noteDeleteRequested);
    connect(m_pianoRoll, &PianoRollView::verticalScrolled,
            this, &PianoRollChartPanel::verticalContentScrolled);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_pianoRoll);
}

void PianoRollChartPanel::setDsPitchDocument(std::shared_ptr<DsPitchDocument> file)
{
    m_pianoRoll->setDsPitchDocument(file);
}

void PianoRollChartPanel::setUndoStack(QUndoStack *stack)
{
    m_pianoRoll->setUndoStack(stack);
}

void PianoRollChartPanel::setBoundaryModel(chart::IBoundaryModel *model)
{
    // Note: PianoRollView doesn't use boundary model directly
    // This is for future integration with NoteBoundaryModel
    ChartPanelBase::setBoundaryModel(model);
}

void PianoRollChartPanel::onViewportUpdate(const chart::ChartCoordinate &conv, int pixelWidth)
{
    ChartPanelBase::onViewportUpdate(conv, pixelWidth);
}

void PianoRollChartPanel::onBoundaryModelInvalidated()
{
    ChartPanelBase::onBoundaryModelInvalidated();
}

void PianoRollChartPanel::paintYAxisContent(QPainter &painter, const QRect &rect)
{
    auto rs = m_pianoRoll->buildRenderState();
    painter.save();
    painter.translate(rect.topLeft());
    PianoRollRenderer::drawPianoKeys(painter, rect.height(), rs);
    painter.restore();
}

void PianoRollChartPanel::renderFullData(QImage &image) {
    Q_UNUSED(image)
    // TODO: F-04 实现完整数据渲染
}

double PianoRollChartPanel::dataDurationSec() const {
    // TODO: F-04 从 PianoRollView/DsPitchDocument 获取实际 duration
    return 0.0;
}

void PianoRollChartPanel::registerChartConfig() {
    static bool registered = false;
    if (registered) return;
    registered = true;

    ChartConfigDescriptor desc;
    desc.chartId = QStringLiteral("pianoroll");
    desc.displayName = QStringLiteral("钢琴卷帘");
    desc.params = {
        {QStringLiteral("pianoWidth"), QStringLiteral("钢琴键宽度"),
         ParamType::Int, QVariant(52), QVariant(30), QVariant(100),
         QStringLiteral(" px"), 0,
         QStringLiteral("左侧钢琴键区域的像素宽度。")},
        {QStringLiteral("scrollBarSize"), QStringLiteral("滚动条宽度"),
         ParamType::Int, QVariant(14), QVariant(8), QVariant(30),
         QStringLiteral(" px"), 0,
         QStringLiteral("垂直滚动条的像素宽度。")},
        {QStringLiteral("minMidi"), QStringLiteral("最低MIDI音高"),
         ParamType::Int, QVariant(24), QVariant(0), QVariant(60),
         QStringLiteral(""), 0,
         QStringLiteral("钢琴卷帘显示的最低MIDI音高。")},
        {QStringLiteral("maxMidi"), QStringLiteral("最高MIDI音高"),
         ParamType::Int, QVariant(96), QVariant(60), QVariant(127),
         QStringLiteral(""), 0,
         QStringLiteral("钢琴卷帘显示的最高MIDI音高。")},
        {QStringLiteral("modSensitivity"), QStringLiteral("调制拖拽灵敏度"),
         ParamType::Double, QVariant(80.0), QVariant(10.0), QVariant(500.0),
         QStringLiteral(""), 1,
         QStringLiteral("调制/漂移拖拽时的灵敏度。数值越大变化越快。")},
        {QStringLiteral("vScale"), QStringLiteral("垂直缩放"),
         ParamType::Double, QVariant(20.0), QVariant(5.0), QVariant(60.0),
         QStringLiteral(" px/半音"), 1,
         QStringLiteral("每个半音对应的像素高度。")},
        {QStringLiteral("boundaryHitRadius"), QStringLiteral("边界点击半径"),
         ParamType::Double, QVariant(5.0), QVariant(2.0), QVariant(20.0),
         QStringLiteral(" px"), 1,
         QStringLiteral("拖动音符边界时的点击检测半径。")},
        {QStringLiteral("defaultResolution"), QStringLiteral("默认分辨率"),
         ParamType::Int, QVariant(40), QVariant(10), QVariant(200),
         QStringLiteral(" px/s"), 0,
         QStringLiteral("默认水平像素/秒分辨率。")},
        {QStringLiteral("showPitchTextOverlay"), QStringLiteral("显示音高文本"),
         ParamType::Int, QVariant(0), QVariant(0), QVariant(1),
         QStringLiteral(""), 0,
         QStringLiteral("是否在音符上显示音高文本覆盖。0=关闭, 1=打开。")},
        {QStringLiteral("showCrosshairAndPitch"), QStringLiteral("显示十字准星"),
         ParamType::Int, QVariant(1), QVariant(0), QVariant(1),
         QStringLiteral(""), 0,
         QStringLiteral("是否显示鼠标位置的十字准星和音高信息。0=关闭, 1=打开。")},
    };
    ChartConfigRegistry::instance().registerChart(desc);
}

void PianoRollChartPanel::loadConfigParams() {
    auto &reg = ChartConfigRegistry::instance();
    m_configPianoWidth = reg.value(QStringLiteral("pianoroll"), QStringLiteral("pianoWidth")).toInt();
    m_configScrollBarSize = reg.value(QStringLiteral("pianoroll"), QStringLiteral("scrollBarSize")).toInt();
    m_configMinMidi = reg.value(QStringLiteral("pianoroll"), QStringLiteral("minMidi")).toInt();
    m_configMaxMidi = reg.value(QStringLiteral("pianoroll"), QStringLiteral("maxMidi")).toInt();
    m_configModSensitivity = reg.value(QStringLiteral("pianoroll"), QStringLiteral("modSensitivity")).toDouble();
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools