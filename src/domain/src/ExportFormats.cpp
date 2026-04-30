#include <dstools/ExportFormats.h>

#include <dstools/DsDocument.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace dstools {

    ExportFormatInfo HtsLabelExportFormat::info() const {
        return {"hts_label", "HTS Label", ".lab", "HTS full-context label format (100ns units)"};
    }

    QStringList HtsLabelExportFormat::requiredInputFormats() const {
        return {"ds"};
    }

    bool HtsLabelExportFormat::exportItem(const QString &sourceFile, const QString & /*workingDir*/,
                                          const QString &outputPath, std::string &error) const {
        auto result = DsDocument::loadFile(sourceFile);
        if (!result) {
            error = result.error();
            return false;
        }

        QFile outFile(outputPath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            error = "Cannot open output file: " + outputPath.toStdString();
            return false;
        }

        QTextStream out(&outFile);
        const auto &doc = *result;

        for (int i = 0; i < doc.sentenceCount(); ++i) {
            const auto &s = doc.sentence(i);
            double offset = DsDocument::numberOrString(s, "offset", 0.0);

            if (!s.contains("ph_seq") || !s.contains("ph_dur"))
                continue;

            QStringList phones = QString::fromStdString(s["ph_seq"].get<std::string>()).split(' ', Qt::SkipEmptyParts);
            QStringList durStrs = QString::fromStdString(s["ph_dur"].get<std::string>()).split(' ', Qt::SkipEmptyParts);

            if (phones.size() != durStrs.size())
                continue;

            double currentTime = offset;
            for (int p = 0; p < phones.size(); ++p) {
                double dur = durStrs[p].toDouble();
                qint64 startHtk = static_cast<qint64>(currentTime * 1e7);
                qint64 endHtk = static_cast<qint64>((currentTime + dur) * 1e7);
                out << startHtk << " " << endHtk << " " << phones[p] << "\n";
                currentTime += dur;
            }
        }

        return true;
    }

    bool HtsLabelExportFormat::exportAll(const QString &workingDir, const QString &outputDir,
                                         std::string &error) const {
        QDir dir(workingDir);
        QDir outDir(outputDir);
        if (!outDir.exists() && !outDir.mkpath(".")) {
            error = "Cannot create output directory: " + outputDir.toStdString();
            return false;
        }

        QStringList dsFiles = dir.entryList({"*.ds"}, QDir::Files);
        if (dsFiles.isEmpty()) {
            error = "No .ds files found in: " + workingDir.toStdString();
            return false;
        }

        for (const auto &fileName : dsFiles) {
            QString src = dir.filePath(fileName);
            QString baseName = QFileInfo(fileName).completeBaseName();
            QString dst = outDir.filePath(baseName + ".lab");
            if (!exportItem(src, workingDir, dst, error))
                return false;
        }
        return true;
    }

    ExportFormatInfo SinsyXmlExportFormat::info() const {
        return {"sinsy_xml", "Sinsy XML", ".xml", "Sinsy-compatible MusicXML format"};
    }

    QStringList SinsyXmlExportFormat::requiredInputFormats() const {
        return {"ds"};
    }

    bool SinsyXmlExportFormat::exportItem(const QString &sourceFile, const QString & /*workingDir*/,
                                          const QString &outputPath, std::string &error) const {
        auto result = DsDocument::loadFile(sourceFile);
        if (!result) {
            error = result.error();
            return false;
        }

        QFile outFile(outputPath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            error = "Cannot open output file: " + outputPath.toStdString();
            return false;
        }

        QTextStream out(&outFile);
        const auto &doc = *result;

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out << "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML 3.1 "
               "Partwise//EN\" \"http://www.musicxml.org/dtds/partwise.dtd\">\n";
        out << "<score-partwise version=\"3.1\">\n";
        out << "  <part-list>\n";
        out << "    <score-part id=\"P1\">\n";
        out << "      <part-name>Voice</part-name>\n";
        out << "    </score-part>\n";
        out << "  </part-list>\n";
        out << "  <part id=\"P1\">\n";

        int measureNum = 1;
        for (int i = 0; i < doc.sentenceCount(); ++i) {
            const auto &s = doc.sentence(i);
            if (!s.contains("note_seq") || !s.contains("note_dur"))
                continue;

            QStringList notes = QString::fromStdString(s["note_seq"].get<std::string>()).split(' ', Qt::SkipEmptyParts);
            QStringList noteDurs =
                QString::fromStdString(s["note_dur"].get<std::string>()).split(' ', Qt::SkipEmptyParts);

            QStringList lyrics;
            if (s.contains("text")) {
                lyrics = QString::fromStdString(s["text"].get<std::string>()).split(' ', Qt::SkipEmptyParts);
            }

            out << "    <measure number=\"" << measureNum++ << "\">\n";
            out << "      <attributes><divisions>480</divisions></attributes>\n";

            for (int n = 0; n < notes.size() && n < noteDurs.size(); ++n) {
                double dur = noteDurs[n].toDouble();
                int divisions = static_cast<int>(dur * 480.0);

                out << "      <note>\n";
                if (notes[n] == "rest" || notes[n] == "SP" || notes[n] == "AP") {
                    out << "        <rest/>\n";
                } else {
                    QString note = notes[n];
                    QString step = note.left(1).toUpper();
                    int alter = 0;
                    int octave = 4;
                    if (note.contains('#')) {
                        alter = 1;
                        octave = note.mid(note.indexOf('#') + 1).toInt();
                    } else if (note.contains('b') && note.length() > 2) {
                        alter = -1;
                        octave = note.mid(note.indexOf('b') + 1).toInt();
                    } else {
                        octave = note.mid(1).toInt();
                    }
                    out << "        <pitch>\n";
                    out << "          <step>" << step << "</step>\n";
                    if (alter != 0)
                        out << "          <alter>" << alter << "</alter>\n";
                    out << "          <octave>" << octave << "</octave>\n";
                    out << "        </pitch>\n";
                }
                out << "        <duration>" << divisions << "</duration>\n";
                if (n < lyrics.size() && !lyrics[n].isEmpty()) {
                    out << "        <lyric><text>" << lyrics[n] << "</text></lyric>\n";
                }
                out << "      </note>\n";
            }
            out << "    </measure>\n";
        }

        out << "  </part>\n";
        out << "</score-partwise>\n";
        return true;
    }

    bool SinsyXmlExportFormat::exportAll(const QString &workingDir, const QString &outputDir,
                                         std::string &error) const {
        QDir dir(workingDir);
        QDir outDir(outputDir);
        if (!outDir.exists() && !outDir.mkpath(".")) {
            error = "Cannot create output directory: " + outputDir.toStdString();
            return false;
        }

        QStringList dsFiles = dir.entryList({"*.ds"}, QDir::Files);
        if (dsFiles.isEmpty()) {
            error = "No .ds files found in: " + workingDir.toStdString();
            return false;
        }

        for (const auto &fileName : dsFiles) {
            QString src = dir.filePath(fileName);
            QString baseName = QFileInfo(fileName).completeBaseName();
            QString dst = outDir.filePath(baseName + ".xml");
            if (!exportItem(src, workingDir, dst, error))
                return false;
        }
        return true;
    }

} // namespace dstools
