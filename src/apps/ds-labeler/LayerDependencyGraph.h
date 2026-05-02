/// @file LayerDependencyGraph.h
/// @brief Directed acyclic graph for tracking layer dependencies and dirty propagation.

#pragma once

#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>

namespace dstools {

/// @brief Tracks dependencies between pipeline layers and propagates dirty flags.
///
/// When a layer is modified, all downstream layers that depend on it are marked
/// dirty. This is persisted per-slice in PipelineContext::dirty.
///
/// Default dependency edges (ADR-57):
///   grapheme → phoneme → ph_num → midi
///   (pitch depends only on audio, not on other layers)
class LayerDependencyGraph {
public:
    LayerDependencyGraph();

    /// Add a dependency edge: downstream depends on upstream.
    void addEdge(const QString &upstream, const QString &downstream);

    /// Given a modified layer, return all transitively downstream layers that
    /// should be marked dirty.
    [[nodiscard]] QStringList dirtyDownstream(const QString &modifiedLayer) const;

    /// Given a set of currently dirty layers, propagate and return the full
    /// transitive dirty set.
    [[nodiscard]] QStringList propagate(const QStringList &dirtyLayers) const;

    /// Return immediate downstream layers.
    [[nodiscard]] QStringList downstream(const QString &layer) const;

    /// Return immediate upstream layers.
    [[nodiscard]] QStringList upstream(const QString &layer) const;

private:
    QMap<QString, QSet<QString>> m_downstream; ///< upstream → set of downstream
    QMap<QString, QSet<QString>> m_upstream;    ///< downstream → set of upstream

    void collectDownstream(const QString &layer, QSet<QString> &visited) const;
};

} // namespace dstools
