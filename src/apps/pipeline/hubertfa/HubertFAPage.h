#pragma once
#include <dstools/TaskWindow.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCharFormat>

namespace HFA {
class HFA;
}

class HubertFAPage : public dstools::widgets::TaskWindow {
    Q_OBJECT
public:
    explicit HubertFAPage(QWidget *parent = nullptr);
    ~HubertFAPage() override;

protected:
    void init() override;
    void runTask() override;
    void onTaskFinished() override;

private slots:
    void slot_outTgPath();
    void slot_browseModel();
    void slot_loadModel();
    void slot_hfaFailed(const QString &filename, const QString &msg);
    void slot_hfaFinished(const QString &filename, const QString &msg);

private:
    QLineEdit *m_outTgEdit;
    QLineEdit *m_modelEdit;
    QLabel *m_modelStatusLabel;
    QPushButton *m_modelLoadBtn;
    QButtonGroup *m_languageGroup = nullptr;
    QHBoxLayout *m_nonSpeechPhLayout = nullptr;
    QWidget *m_dynamicContainer = nullptr;
    HFA::HFA *m_hfa = nullptr;
    QTextCharFormat m_errorFormat;
};
