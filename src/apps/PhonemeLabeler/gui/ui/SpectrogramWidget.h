#pragma once

#include <QWidget>
#include <QImage>
#include <vector>
#include <QPoint>

#include "ViewportController.h"
#include "BoundaryBindingManager.h"
#include "SpectrogramColorPalette.h"

class QPainter;
class QUndoStack;

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

class SpectrogramWidget : public QWidget {
    Q_OBJECT

public:
    explicit SpectrogramWidget(ViewportController *viewport, QWidget *parent = nullptr);
    ~SpectrogramWidget() override;

    void setAudioData(const std::vector<float> &samples, int sampleRate);
    void setViewport(const ViewportState &state);
    void setDocument(TextGridDocument *doc) { m_document = doc; }
    void setBindingManager(BoundaryBindingManager *mgr) { m_bindingMgr = mgr; }
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }
    void setColorPalette(const SpectrogramColorPalette &palette);
    [[nodiscard]] const SpectrogramColorPalette &colorPalette() const { return m_palette; }

    void updateBoundaryOverlay();

signals:
    void boundaryDragStarted(int tierIndex, int boundaryIndex);
    void boundaryDragging(int tierIndex, int boundaryIndex, double newTime);
    void boundaryDragFinished(int tierIndex, int boundaryIndex, double newTime);
    void hoveredBoundaryChanged(int boundaryIndex);
    void entryScrollRequested(int delta);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void computeFullSpectrogram();
    void rebuildViewImage();
    static std::vector<double> makeBlackmanHarrisWindow(int N);
    [[nodiscard]] QRgb intensityToColor(float normalized) const;

    void drawBoundaryOverlay(QPainter &painter);

    // Boundary interaction
    [[nodiscard]] int hitTestBoundary(int x) const;
    void startBoundaryDrag(int boundaryIndex, double time);
    void updateBoundaryDrag(double currentTime);
    void endBoundaryDrag(double finalTime);

    [[nodiscard]] double xToTime(int x) const;
    [[nodiscard]] int timeToX(double time) const;

    ViewportController *m_viewport = nullptr;
    TextGridDocument *m_document = nullptr;
    BoundaryBindingManager *m_bindingMgr = nullptr;
    QUndoStack *m_undoStack = nullptr;

    std::vector<float> m_samples;
    int m_sampleRate = 44100;
    SpectrogramColorPalette m_palette = SpectrogramColorPalette::orangeYellow();

    // Full spectrogram data: [frame][bin] normalized 0..1
    std::vector<std::vector<float>> m_spectrogramData;
    int m_hopSize = 110;
    int m_windowSize = 512;
    int m_numFreqBins = 0;

    // Cached view image
    QImage m_viewImage;
    double m_cachedViewStart = -1.0;
    double m_cachedViewEnd = -1.0;
    int m_cachedWidth = 0;
    int m_cachedHeight = 0;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;

    // Spectrogram parameters (Audacity-style)
    static constexpr int kStandardHopSize = 256;
    static constexpr int kStandardWindowSize = 2048;
    static constexpr double kStandardSampleRate = 44100.0;
    static constexpr double kMaxFrequency = 8000.0;
    static constexpr double kMinIntensityDb = -80.0;
    static constexpr double kMaxIntensityDb = 0.0;

    // Drag state - viewport scrolling
    bool m_dragging = false;
    QPoint m_dragStartPos;
    double m_dragStartTime = 0.0;

    // Drag state - boundary dragging
    bool m_boundaryDragging = false;
    int m_draggedBoundary = -1;
    int m_draggedTier = -1;
    double m_boundaryDragStartTime = 0.0;
    std::vector<AlignedBoundary> m_dragAligned;
    std::vector<double> m_dragAlignedStartTimes;

    static constexpr int kBoundaryHitWidth = 8;
    int m_hoveredBoundary = -1;
};

} // namespace phonemelabeler
} // namespace dstools
