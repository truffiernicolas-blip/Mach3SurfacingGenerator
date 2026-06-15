#include "Mach3PostProcessor.h"

#include <QDateTime>
#include <QLocale>
#include <QTextStream>

QString Mach3PostProcessor::generate(const SurfacingJob &job,
                                     const ToolpathGeometry &geometry,
                                     const QVector<double> &depths) const
{
    QString output;
    QTextStream stream(&output);
    stream.setLocale(QLocale::c());

    stream << "(GENERE PAR MACH3 SURFACING GENERATOR)\n";
    stream << "(DATE: "
           << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
           << ")\n";
    stream << "(T" << job.tool.toolNumber
           << " D=" << number(job.tool.diameter)
           << " - " << job.tool.fluteCount << " DENTS"
           << " - " << commentText(job.tool.name) << ")\n";
    stream << "(MATIERE: " << commentText(job.material.name) << ")\n";
    stream << "(DIMENSIONS: X" << number(job.lengthX)
           << " Y" << number(job.widthY) << " MM)\n";
    stream << "(PROFONDEUR: " << number(job.totalDepth)
           << " MM - PASSE " << number(job.depthPerPass) << " MM)\n";
    stream << "(AVANCE XY: " << number(job.material.feedXY)
           << " MM/MIN - PLONGEE: " << number(job.material.plungeFeed) << " MM/MIN)\n";
    stream << "(BROCHE: " << job.material.spindleSpeed << " TR/MIN)\n";
    stream << "(ORIGINE: "
           << (job.origin == JobOrigin::BottomLeft ? "BAS GAUCHE" : "BAS DROITE")
           << ")\n\n";

    stream << "G90 G94\n";
    stream << "G17\n";
    stream << "G21\n\n";
    stream << "(SURFACAGE)\n";
    stream << "T" << job.tool.toolNumber << "\n";
    stream << "S" << job.material.spindleSpeed << " M3\n";
    stream << "G17 G90 G94\n";
    stream << "G54\n";
    stream << "G0 Z" << number(job.safeZ) << "\n";

    const QPointF start = geometry.points.constFirst();
    for (qsizetype pass = 0; pass < depths.size(); ++pass) {
        stream << "\n(PASSE Z " << pass + 1 << "/" << depths.size()
               << " - Z" << number(depths.at(pass)) << ")\n";
        stream << "G0 Z" << number(job.safeZ) << "\n";
        stream << "G0 X" << number(start.x()) << " Y" << number(start.y()) << "\n";
        stream << "G0 Z" << number(job.approachZ) << "\n";
        stream << "G1 Z" << number(depths.at(pass))
               << " F" << number(job.material.plungeFeed) << "\n";
        stream << "G1 F" << number(job.material.feedXY) << "\n";

        for (qsizetype pointIndex = 1; pointIndex < geometry.points.size(); ++pointIndex) {
            const QPointF point = geometry.points.at(pointIndex);
            stream << "G1 X" << number(point.x()) << " Y" << number(point.y()) << "\n";
        }

        stream << "G0 Z" << number(job.safeZ) << "\n";
    }

    stream << "\nM5\n";
    stream << "M30\n";

    return output;
}

QString Mach3PostProcessor::number(double value)
{
    if (qAbs(value) < 0.0000005) {
        value = 0.0;
    }
    QString formatted = QLocale::c().toString(value, 'f', 3);
    while (formatted.contains(QLatin1Char('.')) && formatted.endsWith(QLatin1Char('0'))) {
        formatted.chop(1);
    }
    if (formatted.endsWith(QLatin1Char('.'))) {
        formatted.chop(1);
    }
    return formatted;
}

QString Mach3PostProcessor::commentText(QString value)
{
    return value.replace(QLatin1Char('('), QLatin1Char('['))
        .replace(QLatin1Char(')'), QLatin1Char(']'))
        .toUpper();
}
