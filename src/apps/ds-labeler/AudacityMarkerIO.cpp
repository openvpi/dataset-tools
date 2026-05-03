#include "AudacityMarkerIO.h"

#include <QFile>
#include <QTextStream>

#include <algorithm>

namespace dstools {
namespace AudacityMarkerIO {

std::vector<double> read(const QString &path, QString &error) {
    std::vector<double> times;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QObject::tr("Cannot open file: %1").arg(path);
        return times;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        QStringList parts = line.split('\t');
        if (parts.size() < 2)
            continue;

        bool ok1 = false, ok2 = false;
        double start = parts[0].toDouble(&ok1);
        double end = parts[1].toDouble(&ok2);

        if (!ok1 || !ok2)
            continue;

        // Use midpoint for region markers, exact time for point markers
        double t = (start == end) ? start : (start + end) / 2.0;
        times.push_back(t);
    }

    std::sort(times.begin(), times.end());
    return times;
}

bool write(const QString &path, const std::vector<double> &times, QString &error) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        error = QObject::tr("Cannot write file: %1").arg(path);
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    out.setRealNumberNotation(QTextStream::FixedNotation);

    for (size_t i = 0; i < times.size(); ++i) {
        QString label = QString("%1").arg(i + 1, 3, 10, QChar('0'));
        out << times[i] << '\t' << times[i] << '\t' << label << '\n';
    }

    return true;
}

} // namespace AudacityMarkerIO
} // namespace dstools
