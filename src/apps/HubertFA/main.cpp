#include "gui/MainWindow.h"
#include <QApplication>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

using namespace HFA;
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

#ifdef Q_OS_WIN
    // For Windows only.
    // Use Win32 API to retrieve the non-client metrics, including the system text font.
    // If success, set application font to system text font.
    NONCLIENTMETRICSW metrics = {sizeof(NONCLIENTMETRICSW)};
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &metrics, 0)) {
        // Get font properties, and construct a `QFont` object
        // Font face
        QString fontFace = QString::fromWCharArray(metrics.lfMessageFont.lfFaceName);

        // Font point size
        qreal fontPointSize = 0.0;
        HDC hDC = GetDC(nullptr);
        if (hDC) {
            // To get font point size, we first get font height (in logical units) and device DPI,
            // then calculate the point size.
            // Here, we use message text font.
            // Reference: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-logfontw
            int dpiY = GetDeviceCaps(hDC, LOGPIXELSY);
            fontPointSize = -72.0 * metrics.lfMessageFont.lfHeight / dpiY;
            ReleaseDC(nullptr, hDC);
        }

        QFont font(fontFace);
        if (fontPointSize > 0) {
            font.setPointSizeF(fontPointSize);
        }
        qApp->setFont(font);
    }
#endif

    MainWindow w;
    w.show();
    return QApplication::exec();
}
