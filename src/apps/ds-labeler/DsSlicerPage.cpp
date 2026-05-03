#include "DsSlicerPage.h"

#include <QLabel>
#include <QMenuBar>
#include <QVBoxLayout>

namespace dstools {

DsSlicerPage::DsSlicerPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    auto *placeholder = new QLabel(
        QStringLiteral("✂️ 切片页面\n\n"
                       "自动切片 + 手动切片编辑 + 切片导出\n\n"
                       "（完整实现将在 M.5 阶段完成）"),
        this);
    placeholder->setAlignment(Qt::AlignCenter);
    auto font = placeholder->font();
    font.setPointSize(14);
    placeholder->setFont(font);
    placeholder->setStyleSheet(QStringLiteral("color: gray;"));

    layout->addStretch();
    layout->addWidget(placeholder);
    layout->addStretch();
}

void DsSlicerPage::setDataSource(ProjectDataSource *source) {
    m_dataSource = source;
}

QMenuBar *DsSlicerPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);

    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
        if (auto *w = window())
            w->close();
    });

    auto *processMenu = bar->addMenu(QStringLiteral("处理(&P)"));
    processMenu->addAction(QStringLiteral("自动切片"))->setEnabled(false);
    processMenu->addAction(QStringLiteral("重新切片"))->setEnabled(false);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("导入切点..."))->setEnabled(false);
    processMenu->addAction(QStringLiteral("保存切点..."))->setEnabled(false);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("导出切片音频..."))->setEnabled(false);

    return bar;
}

QString DsSlicerPage::windowTitle() const {
    return QStringLiteral("DsLabeler — 切片");
}

void DsSlicerPage::onActivated() {
    // Will load audio and display waveform in M.5
}

} // namespace dstools
