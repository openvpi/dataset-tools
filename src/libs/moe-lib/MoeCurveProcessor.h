#pragma once

#include <QObject>
#include <QString>
#include <atomic>
#include <dstools/MouthCurve.h>
#include <dstools/Result.h>
#include <filesystem>
#include <functional>
#include <memory>

namespace Moe {
    class Moe;
}

namespace dstools {

    class MoeCurveProcessor : public QObject {
        Q_OBJECT

    public:
        explicit MoeCurveProcessor(QObject *parent = nullptr);
        ~MoeCurveProcessor() override;

        void setModelPath(const std::filesystem::path &path);

        bool isModelLoaded() const;

        void runInference(const std::filesystem::path &audioPath, std::shared_ptr<std::atomic<bool>> alive);

        std::function<void(const MouthCurve &)> onCurveReady;
        std::function<void(const QString &error)> onError;

    signals:
        void curveReady(const MouthCurve &curve);
        void errorOccurred(const QString &error);

    private:
        std::unique_ptr<Moe::Moe> m_engine;
        std::filesystem::path m_modelPath;
    };

} // namespace dstools