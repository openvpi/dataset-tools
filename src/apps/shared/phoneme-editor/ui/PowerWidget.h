#pragma once

#include "AudioChartWidget.h"
#include "TextGridDocument.h"

#include <vector>

class QPainter;

namespace dstools {
namespace phonemelabeler {

class PowerWidget : public AudioChartWidget {
    Q_OBJECT

public:
    explicit PowerWidget(ViewportController *viewport, QWidget *parent = nullptr);
    ~PowerWidget() override;

    void setAudioData(const std::vector<float> &samples, int sampleRate);
    void setDocument(TextGridDocument *doc);
    [[nodiscard]] TextGridDocument *document() const { return m_document; }

    void rebuildCache() override;

signals:
    void visibleStateChanged(bool visible);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void drawBoundaryOverlay(QPainter &painter);
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void rebuildPowerCache();
    void drawPower(QPainter &painter);
    void drawReferenceLines(QPainter &painter);

    std::vector<float> m_samples;
    int m_sampleRate = 44100;
    TextGridDocument *m_document = nullptr;

    double m_cachedViewStart = -1.0;
    double m_cachedViewEnd = -1.0;
    int m_cachedWidth = 0;
};

} // namespace phonemelabeler
} // namespace dstools