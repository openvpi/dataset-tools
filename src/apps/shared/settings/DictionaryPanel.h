#pragma once

#include <QComboBox>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

namespace dstools {

class DictionaryPanel : public QWidget {
    Q_OBJECT

public:
    explicit DictionaryPanel(QWidget *parent = nullptr);
    ~DictionaryPanel() override = default;

    QWidget *createDictTab();
    QWidget *createPhNumTab();

    QJsonObject collectSettings() const;
    void applySettings(const QJsonObject &data);

    void connectDirtySignals();

signals:
    void dirtyChanged();

private:
    QWidget *createPhNumPathCell();
    QLineEdit *searchLineEditInPathCell(QWidget *cellWidget) const;

    QComboBox *m_g2pEngineCombo = nullptr;
    QLineEdit *m_dictPath = nullptr;
    QLineEdit *m_g2pTestInput = nullptr;
    QPushButton *m_g2pTestBtn = nullptr;
    QLabel *m_g2pTestResult = nullptr;

    QTableWidget *m_phNumTable = nullptr;
    QPushButton *m_phNumAddBtn = nullptr;
    QPushButton *m_phNumRemoveBtn = nullptr;
    QLineEdit *m_phNumSpecialWords = nullptr;
};

} // namespace dstools