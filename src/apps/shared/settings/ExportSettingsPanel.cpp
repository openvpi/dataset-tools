#include "ExportSettingsPanel.h"
#include "AppSettingKeys.h"
#include "SharedSettingsKeys.h"

#include <dsfw/AppSettings.h>

#include <QAbstractItemModel>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QSet>
#include <QSettings>
#include <QVBoxLayout>

namespace dstools {

ExportSettingsPanel::ExportSettingsPanel(QWidget *parent) : QWidget(parent) {}

QWidget *ExportSettingsPanel::createDisplayTab() {
    auto *w = new QWidget(this);
    auto *layout = new QFormLayout(w);

    auto *scaleGroup = new QGroupBox(QStringLiteral("默认比例尺"), w);
    auto *scaleLayout = new QFormLayout(scaleGroup);

    m_defaultResolutionSpin = new QSpinBox(scaleGroup);
    m_defaultResolutionSpin->setRange(0, 50000);
    m_defaultResolutionSpin->setValue(0);
    m_defaultResolutionSpin->setSuffix(QStringLiteral(" spx"));
    m_defaultResolutionSpin->setToolTip(
        QStringLiteral("默认缩放级别（samples per pixel）。\n"
                       "设置为 0 表示打开文件时自动适配窗口。\n"
                       "数值越小放大越近，越大缩小越远。\n"
                       "常用参考值：40（贴近）, 200（适中）, 800（缩远）。"));
    scaleLayout->addRow(QStringLiteral("默认 spx："), m_defaultResolutionSpin);

    static const dstools::SettingsKey<int> kDefaultResolution("AudioVisualizer/defaultResolution", 0);
    static dstools::AppSettings s_avSettings(QStringLiteral("AudioVisualizer"));
    s_avSettings.reload();
    m_defaultResolutionSpin->setValue(s_avSettings.get(kDefaultResolution));

    layout->addWidget(scaleGroup);

    auto *note = new QLabel(
        QStringLiteral("samples per pixel (spx) 控制波形图/频谱图的默认缩放级别。\n"
                       "修改后下次打开新文件时生效。"),
        w);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color: gray; font-style: italic; margin-top: 8px;"));
    layout->addRow(note);

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    return w;
}

QWidget *ExportSettingsPanel::createGeneralTab() {
    auto *w = new QWidget(this);
    auto *layout = new QFormLayout(w);

    m_languageCombo = new QComboBox(w);
    m_languageCombo->addItem(QStringLiteral("跟随系统"), QString());
    m_languageCombo->addItem(QStringLiteral("中文 (zh_CN)"), QStringLiteral("zh_CN"));
    m_languageCombo->addItem(QStringLiteral("English (en)"), QStringLiteral("en"));

    QSettings settings;
    QString savedLang = settings.value(QLatin1String(AppSettingKeys::Language)).toString();
    for (int i = 0; i < m_languageCombo->count(); ++i) {
        if (m_languageCombo->itemData(i).toString() == savedLang) {
            m_languageCombo->setCurrentIndex(i);
            break;
        }
    }

    layout->addRow(QStringLiteral("语言 / Language:"), m_languageCombo);

    auto *note = new QLabel(QStringLiteral("语言更改需要重启应用后生效。\n"
                                           "Language change takes effect after restart."),
                            w);
    note->setStyleSheet(QStringLiteral("color: gray; font-style: italic;"));
    layout->addRow(note);

    layout->addItem(new QSpacerItem(0, 16, QSizePolicy::Minimum, QSizePolicy::Fixed));

    auto *autoSaveGroup = new QGroupBox(QStringLiteral("自动保存"), w);
    auto *autoSaveLayout = new QFormLayout(autoSaveGroup);

    m_autoSaveEnabled = new QCheckBox(QStringLiteral("启用自动保存"), autoSaveGroup);
    m_autoSaveEnabled->setChecked(true);
    autoSaveLayout->addRow(m_autoSaveEnabled);

    m_autoSaveInterval = new QSpinBox(autoSaveGroup);
    m_autoSaveInterval->setRange(10, 300);
    m_autoSaveInterval->setValue(30);
    m_autoSaveInterval->setSuffix(QStringLiteral(" 秒"));
    m_autoSaveInterval->setToolTip(QStringLiteral("自动保存间隔（10-300秒）"));
    autoSaveLayout->addRow(QStringLiteral("间隔："), m_autoSaveInterval);

    dstools::AppSettings appSettings("Editor");
    appSettings.reload();
    m_autoSaveEnabled->setChecked(appSettings.get(dstools::settings::kAutoSaveEnabled));
    m_autoSaveInterval->setValue(appSettings.get(dstools::settings::kAutoSaveIntervalMs) / 1000);

    connect(m_autoSaveEnabled, &QCheckBox::toggled, this, [this]() {
        m_autoSaveInterval->setEnabled(m_autoSaveEnabled->isChecked());
    });

    layout->addRow(autoSaveGroup);

    layout->addItem(new QSpacerItem(0, 16, QSizePolicy::Minimum, QSizePolicy::Fixed));

    auto *chartGroup = new QGroupBox(QStringLiteral("视图布局（子图显隐与顺序）"), w);
    auto *chartLayout = new QVBoxLayout(chartGroup);

    m_chartOrderList = new QListWidget(chartGroup);
    m_chartOrderList->setDragDropMode(QAbstractItemView::InternalMove);
    m_chartOrderList->setDefaultDropAction(Qt::MoveAction);

    QMap<QString, QString> chartNames = {
        {QStringLiteral("waveform"), QStringLiteral("波形图")},
        {QStringLiteral("power"), QStringLiteral("功率图")},
        {QStringLiteral("mouthCurve"), QStringLiteral("口型曲线图")},
        {QStringLiteral("spectrogram"), QStringLiteral("频谱图")},
    };

    static const dstools::SettingsKey<QString> kChartOrder("ViewLayout/chartOrder", "");
    static const dstools::SettingsKey<QString> kChartVisible("ViewLayout/chartVisible", "");
    static dstools::AppSettings s_chartSettings("AudioVisualizer");
    s_chartSettings.reload();
    QString savedOrder = s_chartSettings.get(kChartOrder);
    QString savedVisible = s_chartSettings.get(kChartVisible);
    QSet<QString> visibleSet;
    if (!savedVisible.isEmpty()) {
        const QStringList visibleIds = savedVisible.split(QLatin1Char(','), Qt::SkipEmptyParts);
        visibleSet = QSet<QString>(visibleIds.begin(), visibleIds.end());
    }

    QStringList chartIds;
    if (!savedOrder.isEmpty()) {
        chartIds = savedOrder.split(QLatin1Char(','), Qt::SkipEmptyParts);
    } else {
        chartIds = {QStringLiteral("waveform"), QStringLiteral("power"), QStringLiteral("mouthCurve"),
                    QStringLiteral("spectrogram")};
    }
    for (const auto &id : chartIds) {
        auto *item = new QListWidgetItem(chartNames.value(id, id), m_chartOrderList);
        item->setData(Qt::UserRole, id);
        bool checked;
        if (!savedVisible.isEmpty()) {
            checked = visibleSet.contains(id);
        } else {
            checked = true;
        }
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
    for (auto it = chartNames.constBegin(); it != chartNames.constEnd(); ++it) {
        if (!chartIds.contains(it.key())) {
            auto *item = new QListWidgetItem(it.value(), m_chartOrderList);
            item->setData(Qt::UserRole, it.key());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        }
    }

    chartLayout->addWidget(m_chartOrderList, 1);

    auto *btnRow = new QHBoxLayout;
    m_chartUpBtn = new QPushButton(QStringLiteral("↑ 上移"), chartGroup);
    m_chartDownBtn = new QPushButton(QStringLiteral("↓ 下移"), chartGroup);
    auto *resetBtn = new QPushButton(QStringLiteral("恢复默认"), chartGroup);
    resetBtn->setToolTip(QStringLiteral("恢复为默认显隐状态和排列顺序"));
    btnRow->addWidget(m_chartUpBtn);
    btnRow->addWidget(m_chartDownBtn);
    btnRow->addStretch();
    btnRow->addWidget(resetBtn);
    chartLayout->addLayout(btnRow);

    layout->addRow(chartGroup);

    connect(m_chartUpBtn, &QPushButton::clicked, this, [this]() {
        int row = m_chartOrderList->currentRow();
        if (row > 0) {
            auto *item = m_chartOrderList->takeItem(row);
            m_chartOrderList->insertItem(row - 1, item);
            m_chartOrderList->setCurrentRow(row - 1);
            emit dirtyChanged();
        }
    });
    connect(m_chartDownBtn, &QPushButton::clicked, this, [this]() {
        int row = m_chartOrderList->currentRow();
        if (row >= 0 && row < m_chartOrderList->count() - 1) {
            auto *item = m_chartOrderList->takeItem(row);
            m_chartOrderList->insertItem(row + 1, item);
            m_chartOrderList->setCurrentRow(row + 1);
            emit dirtyChanged();
        }
    });

    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        m_chartOrderList->clear();
        QMap<QString, QString> defaultNames = {
            {QStringLiteral("waveform"), QStringLiteral("波形图")},
            {QStringLiteral("power"), QStringLiteral("功率图")},
            {QStringLiteral("mouthCurve"), QStringLiteral("口型曲线图")},
            {QStringLiteral("spectrogram"), QStringLiteral("频谱图")},
        };
        QStringList defaultOrder = {QStringLiteral("waveform"), QStringLiteral("power"), QStringLiteral("mouthCurve"),
                                    QStringLiteral("spectrogram")};
        for (const auto &id : defaultOrder) {
            auto *item = new QListWidgetItem(defaultNames.value(id, id), m_chartOrderList);
            item->setData(Qt::UserRole, id);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        }
        emit dirtyChanged();
    });

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    return w;
}

QWidget *ExportSettingsPanel::createPreprocessTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);
    auto *placeholder = new QLabel(
        QStringLiteral("预处理配置将在后续版本实现。\n"
                       "计划支持: 响度归一化、降噪。"),
        w);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet(QStringLiteral("color: gray;"));
    layout->addWidget(placeholder);
    return w;
}

void ExportSettingsPanel::collectAndSaveAutoSave() {
    if (m_autoSaveEnabled) {
        static dstools::AppSettings s_editorSettings("Editor");
        s_editorSettings.set(dstools::settings::kAutoSaveEnabled, m_autoSaveEnabled->isChecked());
        s_editorSettings.set(dstools::settings::kAutoSaveIntervalMs, m_autoSaveInterval->value() * 1000);
    }
}

void ExportSettingsPanel::collectAndSaveChartLayout() {
    if (!m_chartOrderList)
        return;

    QStringList order;
    QStringList visible;
    for (int i = 0; i < m_chartOrderList->count(); ++i) {
        auto *item = m_chartOrderList->item(i);
        QString id = item->data(Qt::UserRole).toString();
        order.append(id);
        if (item->checkState() == Qt::Checked)
            visible.append(id);
    }
    static const dstools::SettingsKey<QString> kChartOrder("ViewLayout/chartOrder", "");
    static const dstools::SettingsKey<QString> kChartVisible("ViewLayout/chartVisible", "");
    static dstools::AppSettings s_chartSettings("AudioVisualizer");
    s_chartSettings.set(kChartOrder, order.join(QLatin1Char(',')));
    s_chartSettings.set(kChartVisible, visible.join(QLatin1Char(',')));
}

void ExportSettingsPanel::collectAndSaveLanguage() {
    if (m_languageCombo) {
        QString lang = m_languageCombo->currentData().toString();
        QSettings settings;
        settings.setValue(QLatin1String(AppSettingKeys::Language), lang);
    }
}

void ExportSettingsPanel::collectAndSaveDefaultResolution() {
    if (m_defaultResolutionSpin) {
        static const dstools::SettingsKey<int> kKey("AudioVisualizer/defaultResolution", 0);
        static dstools::AppSettings s(QStringLiteral("AudioVisualizer"));
        s.set(kKey, m_defaultResolutionSpin->value());
    }
}

void ExportSettingsPanel::connectDirtySignals() {
    connect(m_languageCombo, &QComboBox::currentIndexChanged, this, [this]() { emit dirtyChanged(); });
    connect(m_autoSaveEnabled, &QCheckBox::toggled, this, [this]() { emit dirtyChanged(); });
    connect(m_autoSaveInterval, &QSpinBox::valueChanged, this, [this]() { emit dirtyChanged(); });

    connect(m_chartOrderList->model(), &QAbstractItemModel::rowsMoved, this, [this]() { emit dirtyChanged(); });
    connect(m_chartOrderList, &QListWidget::itemChanged, this, [this]() { emit dirtyChanged(); });

    connect(m_defaultResolutionSpin, &QSpinBox::valueChanged, this, [this]() { emit dirtyChanged(); });
}

} // namespace dstools