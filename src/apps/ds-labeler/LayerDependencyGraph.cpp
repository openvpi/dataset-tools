#include "LayerDependencyGraph.h"

namespace dstools {

LayerDependencyGraph::LayerDependencyGraph() {
    // Default dependency edges per ADR-57:
    //   grapheme → phoneme → ph_num → midi
    //   pitch depends only on audio (no layer dependency)
    addEdge(QStringLiteral("grapheme"), QStringLiteral("phoneme"));
    addEdge(QStringLiteral("phoneme"), QStringLiteral("ph_num"));
    addEdge(QStringLiteral("phoneme"), QStringLiteral("midi"));
    addEdge(QStringLiteral("ph_num"), QStringLiteral("midi"));
}

void LayerDependencyGraph::addEdge(const QString &upstream, const QString &downstream) {
    m_downstream[upstream].insert(downstream);
    m_upstream[downstream].insert(upstream);
}

QStringList LayerDependencyGraph::dirtyDownstream(const QString &modifiedLayer) const {
    QSet<QString> visited;
    collectDownstream(modifiedLayer, visited);
    visited.remove(modifiedLayer); // The modified layer itself is not "dirty", it's "changed"
    return QStringList(visited.begin(), visited.end());
}

QStringList LayerDependencyGraph::propagate(const QStringList &dirtyLayers) const {
    QSet<QString> allDirty;
    for (const auto &layer : dirtyLayers) {
        allDirty.insert(layer);
        collectDownstream(layer, allDirty);
    }
    return QStringList(allDirty.begin(), allDirty.end());
}

QStringList LayerDependencyGraph::downstream(const QString &layer) const {
    auto it = m_downstream.find(layer);
    if (it == m_downstream.end())
        return {};
    return QStringList(it->begin(), it->end());
}

QStringList LayerDependencyGraph::upstream(const QString &layer) const {
    auto it = m_upstream.find(layer);
    if (it == m_upstream.end())
        return {};
    return QStringList(it->begin(), it->end());
}

void LayerDependencyGraph::collectDownstream(const QString &layer,
                                              QSet<QString> &visited) const {
    auto it = m_downstream.find(layer);
    if (it == m_downstream.end())
        return;

    for (const auto &child : *it) {
        if (!visited.contains(child)) {
            visited.insert(child);
            collectDownstream(child, visited);
        }
    }
}

} // namespace dstools
