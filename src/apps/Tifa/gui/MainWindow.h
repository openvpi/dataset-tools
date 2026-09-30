#ifndef TIFA_MAINWINDOW_H
#define TIFA_MAINWINDOW_H

#include <AsyncTaskWindow.h>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCharFormat>

#include "../util/Aligner.h"

namespace Tifa {

    class MainWindow : public AsyncTask::AsyncTaskWindow {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow() override;

    protected:
        void init() override;
        void runTask() override;
        void onTaskFinished() override;

    private slots:
        void slot_outTgPath();
        void slot_browseModel();
        void slot_browseDict();
        void slot_loadModel();
        void slot_updateDeviceList();
        void slot_alignFailed(const QString &filename, const QString &msg);
        void slot_alignFinished(const QString &filename, const QString &msg);

    private:
        void appendErrorMessage(const QString &message) const;
        void loadLanguages();
        AlignRequest buildRequest() const;
        static bool checkModelFiles(const QString &modelDir, QString &error);

        QLineEdit *m_outTgEdit = nullptr;
        QLineEdit *m_modelEdit = nullptr;
        QLineEdit *m_dictEdit = nullptr;
        QLabel *m_modelStatusLabel = nullptr;
        QPushButton *m_modelLoadBtn = nullptr;
        QComboBox *m_languageCombo = nullptr;
        QComboBox *m_oovCombo = nullptr;
        QComboBox *m_skipCombo = nullptr;
        QDoubleSpinBox *m_skipPenaltySpin = nullptr;
        QComboBox *m_providerCombo = nullptr;
        QComboBox *m_deviceCombo = nullptr;
        Aligner *m_aligner = nullptr;
        QTextCharFormat m_errorFormat;
    };

} // namespace Tifa

#endif // TIFA_MAINWINDOW_H
