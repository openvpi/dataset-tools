#ifndef HFAWINDOW_H
#define HFAWINDOW_H

#include <AsyncTaskWindow.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCharFormat>

#include "../util/Hfa.h"

namespace HFA {

    class HubertFAWindow : public AsyncTask::AsyncTaskWindow {
        Q_OBJECT
    public:
        explicit HubertFAWindow(QWidget *parent = nullptr);
        ~HubertFAWindow() override;

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
        void appendErrorMessage(const QString &message) const;
        static bool checkModelConfig(const QString &modelDir, QString &error);

        QLineEdit *m_outTgEdit;
        QLineEdit *m_modelEdit;
        QLabel *m_modelStatusLabel;
        QPushButton *m_modelLoadBtn;
        QButtonGroup *m_languageGroup;
        QHBoxLayout *m_nonSpeechPhLayout;
        QWidget *m_dynamicContainer;
        HFA *m_hfa = nullptr;
        QTextCharFormat m_errorFormat;
    };

} // namespace HFA

#endif // HFAWINDOW_H