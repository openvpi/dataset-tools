#include "DsMinLabelPage.h"
#include "ProjectDataSource.h"
#include "SliceListPanel.h"

#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

#include <dsfw/PipelineContext.h>
#include <dsfw/widgets/ToastNotification.h>

namespace dstools {

DsMinLabelPage::DsMinLabelPage(QWidget *parent) : QWidget(parent) {
    m_editor = new Minlabel::MinLabelEditor(this);
    m_sliceList = new SliceListPanel(this);
    m_sliceList->setMinimumWidth(160);
    m_sliceList->setMaximumWidth(280);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_sliceList);
    splitter->addWidget(m_editor);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);

    connect(m_sliceList, &SliceListPanel::sliceSelected,
            this, &DsMinLabelPage::onSliceSelected);
    connect(m_editor, &Minlabel::MinLabelEditor::dataChanged,
            this, [this]() { m_dirty = true; });
}

void DsMinLabelPage::setDataSource(ProjectDataSource *source) {
    m_source = source;
    m_sliceList->setDataSource(source);
}

void DsMinLabelPage::onSliceSelected(const QString &sliceId) {
    if (!maybeSave())
        return;

    m_currentSliceId = sliceId;

    if (!m_source)
        return;

    // Load slice data
    auto result = m_source->loadSlice(sliceId);
    if (result) {
        const auto &doc = result.value();
        // Extract grapheme layer text as lab content
        QString labContent;
        QString rawText;
        for (const auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("grapheme")) {
                QStringList parts;
                for (const auto &b : layer.boundaries) {
                    if (!b.text.isEmpty())
                        parts << b.text;
                }
                labContent = parts.join(QStringLiteral(" "));
            } else if (layer.name == QStringLiteral("raw_text")) {
                QStringList parts;
                for (const auto &b : layer.boundaries) {
                    if (!b.text.isEmpty())
                        parts << b.text;
                }
                rawText = parts.join(QStringLiteral(" "));
            }
        }
        m_editor->loadData(labContent, rawText);
    } else {
        m_editor->loadData({}, {});
    }

    // Load audio
    const QString audio = m_source->audioPath(sliceId);
    m_editor->setAudioFile(audio);

    m_dirty = false;
    emit sliceChanged(sliceId);
}

bool DsMinLabelPage::saveCurrentSlice() {
    if (m_currentSliceId.isEmpty() || !m_source || !m_dirty)
        return true;

    // Build DsTextDocument from editor content
    auto result = m_source->loadSlice(m_currentSliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

    // Update grapheme layer
    IntervalLayer *graphemeLayer = nullptr;
    for (auto &layer : doc.layers) {
        if (layer.name == QStringLiteral("grapheme")) {
            graphemeLayer = &layer;
            break;
        }
    }
    if (!graphemeLayer) {
        doc.layers.push_back({});
        graphemeLayer = &doc.layers.back();
        graphemeLayer->name = QStringLiteral("grapheme");
        graphemeLayer->type = QStringLiteral("text");
    }

    // Store lab content as boundaries
    graphemeLayer->boundaries.clear();
    const QString lab = m_editor->labContent();
    if (!lab.isEmpty()) {
        const auto parts = lab.split(QChar(' '), Qt::SkipEmptyParts);
        int id = 1;
        for (const auto &part : parts) {
            Boundary b;
            b.id = id++;
            b.text = part;
            graphemeLayer->boundaries.push_back(std::move(b));
        }
    }

    auto saveResult = m_source->saveSlice(m_currentSliceId, doc);
    if (!saveResult) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QString::fromStdString(saveResult.error()));
        return false;
    }

    m_dirty = false;
    return true;
}

bool DsMinLabelPage::maybeSave() {
    if (!m_dirty)
        return true;

    auto ret = QMessageBox::question(
        this, QStringLiteral("未保存的更改"),
        QStringLiteral("当前切片已修改，是否保存？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
        return saveCurrentSlice();
    if (ret == QMessageBox::Discard) {
        m_dirty = false;
        return true;
    }
    return false; // Cancel
}

QMenuBar *DsMinLabelPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);

    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(QStringLiteral("保存"), this, [this]() { saveCurrentSlice(); },
                        QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
        if (auto *w = window()) w->close();
    });

    auto *processMenu = bar->addMenu(QStringLiteral("处理(&P)"));
    processMenu->addAction(QStringLiteral("ASR 识别当前曲目"), this, &DsMinLabelPage::onRunAsr);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("批量 ASR..."), this, &DsMinLabelPage::onBatchAsr);

    return bar;
}

QString DsMinLabelPage::windowTitle() const {
    QString title = QStringLiteral("DsLabeler — 歌词标注");
    if (!m_currentSliceId.isEmpty())
        title += QStringLiteral(" — ") + m_currentSliceId;
    return title;
}

bool DsMinLabelPage::hasUnsavedChanges() const {
    return m_dirty;
}

void DsMinLabelPage::onActivated() {
    // Refresh slice list when page becomes active
    m_sliceList->refresh();

    // M.3.11: Check dirty layers for current slice
    if (m_source && !m_currentSliceId.isEmpty()) {
        auto *ctx = m_source->context(m_currentSliceId);
        if (ctx && ctx->dirty.contains(QStringLiteral("grapheme"))) {
            // Placeholder: clear dirty flag (actual recalculation requires inference)
            ctx->dirty.removeAll(QStringLiteral("grapheme"));
            m_source->saveContext(m_currentSliceId);
            dsfw::widgets::ToastNotification::show(
                this, dsfw::widgets::ToastType::Info,
                QStringLiteral("歌词层已更新"), 3000);
        }
    }
}

bool DsMinLabelPage::onDeactivating() {
    return maybeSave();
}

void DsMinLabelPage::onRunAsr() {
    // TODO: Integrate FunASR inference for current item
    QMessageBox::information(this, QStringLiteral("ASR"),
                             QStringLiteral("ASR 功能将在推理库集成后实现。"));
}

void DsMinLabelPage::onBatchAsr() {
    // TODO: Batch ASR processing
    QMessageBox::information(this, QStringLiteral("批量 ASR"),
                             QStringLiteral("批量 ASR 功能将在推理库集成后实现。"));
}

} // namespace dstools
