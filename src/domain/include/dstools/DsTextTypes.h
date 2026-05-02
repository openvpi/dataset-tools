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

struct DsTextMeta {
    QStringList editedSteps;
};

struct DsTextDocument {
    QString version = "3.0.0";
    DsTextAudio audio;
    std::vector<IntervalLayer> layers;
    std::vector<CurveLayer> curves;
    std::vector<BindingGroup> groups;
    DsTextMeta meta;

    static Result<DsTextDocument> load(const QString &path);
    Result<void> save(const QString &path) const;

    const BindingGroup *findGroup(int boundaryId) const;
    std::vector<int> boundIdsOf(int boundaryId) const;
};

} // namespace dstools
