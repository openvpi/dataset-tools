#include <dstools/AudacityMarkerAdapter.h>

#include <QFile>
#include <QTextStream>

#include <algorithm>

namespace dstools {

Result<std::vector<double>> AudacityMarkerAdapter::read(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return Result<std::vector<double>>::Error(
            QObject::tr("Cannot open file: %1").arg(path).toStdString());
    }

    std::vector<double> times;
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

        double t = (start == end) ? start : (start + end) / 2.0;
        times.push_back(t);
    }

    std::sort(times.begin(), times.end());
    return times;
}

Result<void> AudacityMarkerAdapter::write(const QString &path, const std::vector<double> &times) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return Result<void>::Error(
            QObject::tr("Cannot write file: %1").arg(path).toStdString());
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    out.setRealNumberNotation(QTextStream::FixedNotation);

    for (size_t i = 0; i < times.size(); ++i) {
        QString label = QString("%1").arg(i + 1, 3, 10, QChar('0'));
        out << times[i] << '\t' << times[i] << '\t' << label << '\n';
    }

    return Result<void>::Ok();
}

} // namespace dstools