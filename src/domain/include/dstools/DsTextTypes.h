#pragma once

#include <QString>
#include <QStringList>
#include <dsfw/Result.h>
#include <dsfw/TimePos.h>
#include <dstools/Version.h>
#include <vector>

namespace dstools {

inline constexpr const char* kDsTextVersion = DSTOOLS_DOMAIN_VERSION;
inline constexpr const char* kDsTextVersionFallback = "2.0.0";

struct Boundary {
    int id = 0;
    dsfw::TimePos pos = 0;
    QString text;
};

struct IntervalLayer {
    QString name;
    QString type;  ///< "text" or "note"
    std::vector<Boundary> boundaries;

    void sortBoundaries();
    int nextId() const;
};

struct CurveLayer {
    QString name;
    QString type = QStringLiteral("curve");
    dsfw::TimePos timestep = 0;
    std::vector<float> values;
};

struct DsTextAudio {
    QString path;
    dsfw::TimePos in = 0;
    dsfw::TimePos out = 0;
};

using BindingGroup = std::vector<int>;

struct LayerDependency {
    int parentLayerIndex = -1;
    int childLayerIndex = -1;
    QString parentLayerName;
    QString childLayerName;
    int parentStartBoundaryId = -1;
    int parentEndBoundaryId = -1;
    int childStartBoundaryId = -1;
    int childEndBoundaryId = -1;
    std::vector<int> childBoundaryIds;
};

struct DsTextMeta {
    QStringList editedSteps;
};

struct DsTextDocument {
    QString version = QString::fromLatin1(kDsTextVersion);
    DsTextAudio audio;
    std::vector<IntervalLayer> layers;
    std::vector<CurveLayer> curves;
    std::vector<BindingGroup> groups;
    std::vector<LayerDependency> dependencies;
    DsTextMeta meta;

    static dsfw::Result<DsTextDocument> load(const QString& path);
    dsfw::Result<void> save(const QString& path) const;
    [[nodiscard]] dsfw::Result<void> validate() const;

    const BindingGroup* findGroup(int boundaryId) const;
    std::vector<int> boundIdsOf(int boundaryId) const;
};

}  // namespace dstools
