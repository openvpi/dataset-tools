#pragma once

#include <dstools/TimePos.h>
#include <dstools/Result.h>
#include <QString>
#include <QStringList>
#include <vector>

namespace dstools {

struct Boundary {
    int id = 0;
    TimePos pos = 0;
    QString text;
};

struct IntervalLayer {
    QString name;
    QString type;       ///< "text" or "note"
    std::vector<Boundary> boundaries;

    void sortBoundaries();
    int nextId() const;
};

struct CurveLayer {
    QString name;
    QString type = QStringLiteral("curve");
    TimePos timestep = 0;
    std::vector<int32_t> values;
};

struct DsTextAudio {
    QString path;
    TimePos in = 0;
    TimePos out = 0;
};

using BindingGroup = std::vector<int>;

struct LayerDependency {
    int parentLayerIndex = -1;
    int childLayerIndex = -1;
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
    QString version = "3.0.0";
    DsTextAudio audio;
    std::vector<IntervalLayer> layers;
    std::vector<CurveLayer> curves;
    std::vector<BindingGroup> groups;
    std::vector<LayerDependency> dependencies;
    DsTextMeta meta;

    static Result<DsTextDocument> load(const QString &path);
    Result<void> save(const QString &path) const;

    const BindingGroup *findGroup(int boundaryId) const;
    std::vector<int> boundIdsOf(int boundaryId) const;
};

} // namespace dstools
