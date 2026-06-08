#include "FileDataSource.h"

#include <dsfw/FormatAdapterRegistry.h>
#include <dsfw/IFormatAdapter.h>
#include <dstools/DsTextTypes.h>
#include <dstools/DsKeys.h>
#include <dsfw/TimePos.h>

#include <QFileInfo>

#include <map>

namespace dstools {


namespace {

QString formatIdFromExtension(const QString& filePath) {
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == QStringLiteral("textgrid"))
        return QStringLiteral("textgrid");
    if (ext == QStringLiteral("lab"))
        return QStringLiteral("lab");
    if (ext == QStringLiteral("ds"))
        return QStringLiteral("ds");
    if (ext == QStringLiteral("csv"))
        return QStringLiteral("csv");
    return {};
}

dsfw::Result<dsfw::DsTextDocument> layersToDocument(const std::map<QString, LayerData>& layers, const QString& audioPath) {
    dsfw::DsTextDocument doc;
    doc.audio.path = audioPath;

    for (const auto& [key, layerData] : layers) {
        const nlohmann::json value = layerData.toJson();
        if (value.is_object() && value.contains("boundaries")) {
            IntervalLayer layer;
            layer.name = key;
            if (key == QString::fromUtf8(dstools::keys::layers::midi) ||
                key == QString::fromUtf8(dstools::keys::layers::note))
                layer.type = QString::fromUtf8(dstools::keys::layers::note);
            else
                layer.type = QStringLiteral("text");

            for (const auto& jb : value["boundaries"]) {
                Boundary b;
                b.id = jb.value("id", 0);
                b.pos = jb.value("pos", int64_t(0));
                b.text = QString::fromStdString(jb.value("text", ""));
                layer.boundaries.push_back(b);
            }
            layer.sortBoundaries();
            doc.layers.push_back(std::move(layer));
        } else if (value.is_object() && value.contains("f0_seq")) {
            CurveLayer curve;
            curve.name = key;
            curve.type = QStringLiteral("curve");

            double timestepSec = value.value("f0_timestep", 0.005);
            curve.timestep = secToUs(timestepSec);

            if (value["f0_seq"].is_string()) {
                const auto f0Str = QString::fromStdString(value["f0_seq"].get<std::string>());
                const QStringList vals = f0Str.split(' ', Qt::SkipEmptyParts);
                curve.values.reserve(vals.size());
                for (const auto& v : vals)
                    curve.values.push_back(hzToMhz(v.toDouble()));
            }

            doc.curves.push_back(std::move(curve));
        }
    }

    return dsfw::Result<dsfw::DsTextDocument>::Ok(std::move(doc));
}

void documentToLayers(const dsfw::DsTextDocument& doc, std::map<QString, LayerData>& layers) {
    for (const auto& layer : doc.layers) {
        nlohmann::json layerJson;
        layerJson["name"] = layer.name.toStdString();

        nlohmann::json boundaries = nlohmann::json::array();
        for (const auto& b : layer.boundaries) {
            boundaries.push_back({{"id", b.id}, {"pos", b.pos}, {"text", b.text.toStdString()}});
        }
        layerJson["boundaries"] = boundaries;
        layers[layer.name] = LayerData::fromJson(layerJson);
    }

    for (const auto& curve : doc.curves) {
        nlohmann::json curveJson;
        curveJson["name"] = curve.name.toStdString();

        QStringList f0Vals;
        f0Vals.reserve(static_cast<int>(curve.values.size()));
        for (const auto& v : curve.values)
            f0Vals.append(QString::number(mhzToHz(v), 'f', 2));
        curveJson["f0_seq"] = f0Vals.join(' ').toStdString();

        double timestepSec = usToSec(curve.timestep);
        curveJson["f0_timestep"] = timestepSec;

        layers[curve.name] = LayerData::fromJson(curveJson);
    }
}

} // namespace

FileDataSource::FileDataSource(QObject* parent) : dsfw::IEditorDataSource(parent) {
}

int FileDataSource::getSliceCount() const {
    return m_annotationPath.isEmpty() ? 0 : 1;
}

void FileDataSource::setFile(const QString& audioFilePath, const QString& annotationPath) {
    m_audioPath = audioFilePath;
    m_annotationPath = annotationPath;
    m_formatId = formatIdFromExtension(annotationPath);
    emit sliceListChanged();
}

void FileDataSource::clear() {
    m_audioPath.clear();
    m_annotationPath.clear();
    m_formatId.clear();
    emit sliceListChanged();
}

bool FileDataSource::hasFile() const {
    return !m_annotationPath.isEmpty();
}

QStringList FileDataSource::sliceIds() const {
    if (m_annotationPath.isEmpty())
        return {};
    return {m_annotationPath};
}

dsfw::Result<dsfw::DsTextDocument> FileDataSource::loadSlice(const QString& sliceId) {
    Q_UNUSED(sliceId)

    if (m_formatId.isEmpty())
        return DsTextDocument::load(m_annotationPath);

    auto* adapter = dsfw::FormatAdapterRegistry::instance().adapter(m_formatId);
    if (!adapter || !adapter->canImport())
        return DsTextDocument::load(m_annotationPath);

    std::map<QString, LayerData> layers;
    auto result = adapter->importToLayers(m_annotationPath, layers, {});
    if (!result) {
        return dsfw::Result<dsfw::DsTextDocument>::Error(result.error());
    }

    auto docResult = layersToDocument(layers, m_audioPath);
    if (!docResult) {
        return dsfw::Result<dsfw::DsTextDocument>::Error(docResult.error());
    }

    auto validationResult = docResult.value().validate();
    if (!validationResult) {
        return dsfw::Result<dsfw::DsTextDocument>::Error("Imported document validation failed: " + validationResult.error());
    }

    return docResult;
}

dsfw::Result<void> FileDataSource::saveSlice(const QString& sliceId, const dsfw::DsTextDocument& doc) {
    Q_UNUSED(sliceId)

    if (m_formatId.isEmpty()) {
        auto result = doc.save(m_annotationPath);
        if (result)
            emit sliceSaved(m_annotationPath);
        return result;
    }

    auto* adapter = dsfw::FormatAdapterRegistry::instance().adapter(m_formatId);
    if (!adapter || !adapter->canExport()) {
        QFileInfo fi(m_annotationPath);
        QString dstextPath = fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName() + QStringLiteral(".dstext");
        auto result = doc.save(dstextPath);
        if (result)
            emit sliceSaved(m_annotationPath);
        return result;
    }

    std::map<QString, LayerData> layers;
    documentToLayers(doc, layers);

    auto result = adapter->exportFromLayers(layers, m_annotationPath, {});
    if (result)
        emit sliceSaved(m_annotationPath);
    return result;
}

QString FileDataSource::audioPath(const QString& sliceId) const {
    Q_UNUSED(sliceId)
    return m_audioPath;
}

} // namespace dstools
