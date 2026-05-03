#include "DsPhonemeLabelerPage.h"
#include "ProjectDataSource.h"
#include "SliceListPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

#include <dsfw/PipelineContext.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/DsTextTypes.h>
#include <ui/TextGridDocument.h>

namespace dstools {

DsPhonemeLabelerPage::DsPhonemeLabelerPage(QWidget *parent) : QWidget(parent) {
    m_editor = new phonemelabeler::PhonemeEditor(this);
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
            this, &DsPhonemeLabelerPage::onSliceSelected);
    connect(m_editor->saveAction(), &QAction::triggered,
            this, [this]() { saveCurrentSlice(); });
}

void DsPhonemeLabelerPage::setDataSource(ProjectDataSource *source) {
    m_source = source;
    m_sliceList->setDataSource(source);
}

void DsPhonemeLabelerPage::onSliceSelected(const QString &sliceId) {
    if (!maybeSave())
        return;

    m_currentSliceId = sliceId;

    if (!m_source)
        return;

    // Load TextGrid from dstext
    const QString audioPath = m_source->audioPath(sliceId);

    // Load the dstext and try to find phoneme data to build a TextGrid
    auto result = m_source->loadSlice(sliceId);
    if (result) {
        // Create a temporary TextGrid from dstext layers for editing
        // The PhonemeEditor expects a TextGridDocument loaded from file
        // For now, load audio directly - TextGrid creation from dstext is future work
    }

    // Load audio into editor
    if (!audioPath.isEmpty())
        m_editor->loadAudio(audioPath);

    emit sliceChanged(sliceId);
}

bool DsPhonemeLabelerPage::saveCurrentSlice() {
    if (m_currentSliceId.isEmpty() || !m_source)
        return true;

    if (!m_editor->document()->isModified())
        return true;

    auto *doc = m_editor->document();
    auto result = m_source->loadSlice(m_currentSliceId);
    DsTextDocument dstext;
    if (result)
        dstext = std::move(result.value());

    bool foundPhonemeLayer = false;
    for (int t = 0; t < doc->tierCount(); ++t) {
        QString tierName = doc->tierName(t);
        if (tierName.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive) ||
            tierName.contains(QStringLiteral("phone"), Qt::CaseInsensitive)) {
            IntervalLayer layer;
            layer.name = tierName;
            layer.type = QStringLiteral("text");
            for (int i = 0; i < doc->intervalCount(t); ++i) {
                Boundary b;
                b.pos = doc->intervalStart(t, i);
                b.text = doc->intervalText(t, i);
                b.id = i;
                layer.boundaries.push_back(std::move(b));
            }
            layer.sortBoundaries();

            bool replaced = false;
            for (auto &existing : dstext.layers) {
                if (existing.name == layer.name) {
                    existing = std::move(layer);
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
                dstext.layers.push_back(std::move(layer));
            foundPhonemeLayer = true;
            break;
        }
    }

    if (foundPhonemeLayer) {
        auto saveResult = m_source->saveSlice(m_currentSliceId, dstext);
        if (!saveResult) {
            QMessageBox::warning(this, QStringLiteral("保存失败"),
                                 QString::fromStdString(saveResult.error()));
            return false;
        }
    } else {
        if (!doc->save(
                m_source->audioPath(m_currentSliceId).replace(
                    QStringLiteral(".wav"), QStringLiteral(".TextGrid")))) {
            QMessageBox::warning(this, QStringLiteral("保存失败"),
                                 QStringLiteral("无法保存 TextGrid 文件。"));
            return false;
        }
    }

    return true;
}

bool DsPhonemeLabelerPage::maybeSave() {
    if (!m_editor->document() || !m_editor->document()->isModified())
        return true;

    auto ret = QMessageBox::question(
        this, QStringLiteral("未保存的更改"),
        QStringLiteral("当前切片已修改，是否保存？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
        return saveCurrentSlice();
    if (ret == QMessageBox::Discard)
        return true;
    return false;
}

QMenuBar *DsPhonemeLabelerPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);

    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(m_editor->saveAction());
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
        if (auto *w = window()) w->close();
    });

    auto *editMenu = bar->addMenu(QStringLiteral("编辑(&E)"));
    editMenu->addAction(m_editor->undoAction());
    editMenu->addAction(m_editor->redoAction());

    auto *processMenu = bar->addMenu(QStringLiteral("处理(&P)"));
    processMenu->addAction(QStringLiteral("强制对齐当前切片"), this, &DsPhonemeLabelerPage::onRunFA);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("批量强制对齐..."), this, &DsPhonemeLabelerPage::onBatchFA);

    auto *viewMenu = bar->addMenu(QStringLiteral("视图(&V)"));
    for (auto *act : m_editor->viewActions()) {
        if (act)
            viewMenu->addAction(act);
        else
            viewMenu->addSeparator();
    }
    viewMenu->addMenu(m_editor->spectrogramColorMenu());
    viewMenu->addSeparator();
    dsfw::Theme::instance().populateThemeMenu(viewMenu);

    return bar;
}

QWidget *DsPhonemeLabelerPage::createStatusBarContent(QWidget *parent) {
    auto *container = new QWidget(parent);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *sliceLabel = new QLabel(QStringLiteral("未选择切片"), container);
    auto *posLabel = new QLabel(QStringLiteral("0.000s"), container);
    layout->addWidget(sliceLabel, 1);
    layout->addWidget(posLabel);

    connect(this, &DsPhonemeLabelerPage::sliceChanged, sliceLabel, [sliceLabel](const QString &id) {
        sliceLabel->setText(id.isEmpty() ? QStringLiteral("未选择切片") : id);
    });
    connect(m_editor, &phonemelabeler::PhonemeEditor::positionChanged,
            this, [posLabel](double sec) {
        posLabel->setText(QString::number(sec, 'f', 3) + "s");
    });

    return container;
}

QString DsPhonemeLabelerPage::windowTitle() const {
    QString title = QStringLiteral("DsLabeler — 音素标注");
    if (!m_currentSliceId.isEmpty())
        title += QStringLiteral(" — ") + m_currentSliceId;
    return title;
}

bool DsPhonemeLabelerPage::hasUnsavedChanges() const {
    return m_editor->document() && m_editor->document()->isModified();
}

void DsPhonemeLabelerPage::onActivated() {
    m_sliceList->refresh();

    // M.3.11: Check dirty layers for current slice
    if (m_source && !m_currentSliceId.isEmpty()) {
        auto *ctx = m_source->context(m_currentSliceId);
        if (ctx && ctx->dirty.contains(QStringLiteral("phoneme"))) {
            dsfw::widgets::ToastNotification::show(
                this, dsfw::widgets::ToastType::Warning,
                QStringLiteral("音素层数据已过期（上游歌词已修改），建议重新对齐"),
                3000);
            // Placeholder: clear dirty flag (actual re-alignment requires inference)
            ctx->dirty.removeAll(QStringLiteral("phoneme"));
            m_source->saveContext(m_currentSliceId);
        }
    }
}

bool DsPhonemeLabelerPage::onDeactivating() {
    return maybeSave();
}

void DsPhonemeLabelerPage::onRunFA() {
    QMessageBox::information(this, QStringLiteral("强制对齐"),
                             QStringLiteral("强制对齐功能将在推理库集成后实现。"));
}

void DsPhonemeLabelerPage::onBatchFA() {
    QMessageBox::information(this, QStringLiteral("批量强制对齐"),
                             QStringLiteral("批量强制对齐功能将在推理库集成后实现。"));
}

} // namespace dstools
