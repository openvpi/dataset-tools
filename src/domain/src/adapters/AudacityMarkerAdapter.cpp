#include <dstools/AudacityMarkerAdapter.h>

#include <QFile>
#include <QTextStream>

#include <algorithm>

#include <dsfw/AtomicFileWriter.h>
#include <dsfw/PathUtils.h>

namespace dstools {

using namespace dsfw;

dsfw::Result<std::vector<double>> AudacityMarkerAdapter::read(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return dsfw::Result<std::vector<double>>::Error(QObject::tr("Cannot open file: %1").arg(path).toStdString());
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

dsfw::Result<void> AudacityMarkerAdapter::write(const QString& path, const std::vector<double>& times) {
    QString content;
    QTextStream out(&content);
    out.setRealNumberPrecision(6);
    out.setRealNumberNotation(QTextStream::FixedNotation);

    for (size_t i = 0; i < times.size(); ++i) {
        QString label = QString("%1").arg(i + 1, 3, 10, QChar('0'));
        out << times[i] << '\t' << times[i] << '\t' << label << '\n';
    }

    const std::string data = content.toStdString();
    return dsfw::AtomicFileWriter::write(dsfw::PathUtils::toStdPath(path), data);
}

}  // namespace dstools