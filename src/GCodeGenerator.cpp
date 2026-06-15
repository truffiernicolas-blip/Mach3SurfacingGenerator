#include "GCodeGenerator.h"

#include "Mach3PostProcessor.h"

#include <QtMath>

#include <algorithm>
#include <cmath>

QStringList GCodeGenerator::validate(const SurfacingJob &job) const
{
    QStringList errors;

    if (job.lengthX <= 0.0) {
        errors << QStringLiteral("La longueur X doit être supérieure à 0.");
    }
    if (job.widthY <= 0.0) {
        errors << QStringLiteral("La largeur Y doit être supérieure à 0.");
    }
    if (job.totalDepth <= 0.0) {
        errors << QStringLiteral("La profondeur totale doit être supérieure à 0.");
    }
    if (job.depthPerPass <= 0.0) {
        errors << QStringLiteral("La profondeur par passe doit être supérieure à 0.");
    }
    if (job.outsideMargin < 0.0) {
        errors << QStringLiteral("La marge extérieure ne peut pas être négative.");
    }
    if (job.tool.diameter <= 0.0) {
        errors << QStringLiteral("Le diamètre de l'outil doit être supérieur à 0.");
    }
    if (job.tool.toolNumber <= 0) {
        errors << QStringLiteral("Le numéro d'outil doit être supérieur à 0.");
    }
    if (job.tool.fluteCount <= 0) {
        errors << QStringLiteral("Le nombre de dents doit être supérieur à 0.");
    }
    if (job.tool.overlapPercent < 0.0 || job.tool.overlapPercent >= 100.0) {
        errors << QStringLiteral("Le recouvrement doit être compris entre 0 et 100 % exclus.");
    }
    if (job.tool.stepover() <= 0.0 || job.tool.stepover() > job.tool.diameter) {
        errors << QStringLiteral("Le stepover doit être supérieur à 0 et inférieur ou égal au diamètre.");
    }
    if (job.material.feedXY <= 0.0) {
        errors << QStringLiteral("L'avance XY doit être supérieure à 0.");
    }
    if (job.material.plungeFeed <= 0.0) {
        errors << QStringLiteral("L'avance de plongée doit être supérieure à 0.");
    }
    if (job.material.spindleSpeed <= 0) {
        errors << QStringLiteral("La vitesse de broche doit être supérieure à 0.");
    }
    if (job.safeZ <= job.approachZ) {
        errors << QStringLiteral("Le Z sécurité doit être strictement supérieur au Z approche.");
    }
    if (job.approachZ < 0.0) {
        errors << QStringLiteral("Le Z approche doit être positif ou nul.");
    }
    if (job.outputFileName.trimmed().isEmpty()) {
        errors << QStringLiteral("Le nom du fichier de sortie est obligatoire.");
    }

    return errors;
}

ToolpathGeometry GCodeGenerator::buildGeometry(const SurfacingJob &job) const
{
    ToolpathGeometry geometry;

    const bool rightOrigin = job.origin == JobOrigin::BottomRight;
    const double radiusAndMargin = job.tool.diameter / 2.0 + job.outsideMargin;

    const double workpieceLeft = rightOrigin ? -job.lengthX : 0.0;
    const double workpieceRight = rightOrigin ? 0.0 : job.lengthX;
    geometry.workpiece = QRectF(QPointF(workpieceLeft, 0.0),
                                QPointF(workpieceRight, job.widthY)).normalized();
    geometry.origin = QPointF(0.0, 0.0);

    const double xMin = geometry.workpiece.left() - radiusAndMargin;
    const double xMax = geometry.workpiece.right() + radiusAndMargin;
    const double yMin = geometry.workpiece.top() - radiusAndMargin;
    const double yMax = geometry.workpiece.bottom() + radiusAndMargin;
    geometry.centerBounds = QRectF(QPointF(xMin, yMin), QPointF(xMax, yMax)).normalized();

    const double stepover = job.tool.stepover();

    if (job.sweepDirection == SweepDirection::AlongX) {
        const double span = yMax - yMin;
        geometry.scanLineCount = std::max(2, qCeil(span / stepover) + 1);

        for (int line = 0; line < geometry.scanLineCount; ++line) {
            const double y = line == geometry.scanLineCount - 1
                ? yMax
                : std::min(yMin + line * stepover, yMax);
            const bool forward = (line % 2 == 0) != rightOrigin;
            geometry.points << QPointF(forward ? xMin : xMax, y)
                            << QPointF(forward ? xMax : xMin, y);
        }
    } else {
        const double span = xMax - xMin;
        geometry.scanLineCount = std::max(2, qCeil(span / stepover) + 1);

        for (int line = 0; line < geometry.scanLineCount; ++line) {
            const double x = line == geometry.scanLineCount - 1
                ? (rightOrigin ? xMin : xMax)
                : (rightOrigin ? std::max(xMax - line * stepover, xMin)
                               : std::min(xMin + line * stepover, xMax));
            const bool forward = line % 2 == 0;
            geometry.points << QPointF(x, forward ? yMin : yMax)
                            << QPointF(x, forward ? yMax : yMin);
        }
    }

    for (qsizetype i = 1; i < geometry.points.size(); ++i) {
        const QPointF delta = geometry.points.at(i) - geometry.points.at(i - 1);
        geometry.xyLengthPerPass += std::hypot(delta.x(), delta.y());
    }

    return geometry;
}

QVector<double> GCodeGenerator::buildDepths(const SurfacingJob &job) const
{
    QVector<double> depths;
    double removed = 0.0;

    while (removed + 1e-9 < job.totalDepth) {
        removed = std::min(removed + job.depthPerPass, job.totalDepth);
        depths << -removed;
    }

    return depths;
}

JobSummary GCodeGenerator::summarize(const SurfacingJob &job,
                                     const ToolpathGeometry &geometry) const
{
    const QVector<double> depths = buildDepths(job);
    JobSummary summary;
    summary.depthPassCount = depths.size();
    summary.scanLineCount = geometry.scanLineCount;
    summary.finalDepth = depths.isEmpty() ? 0.0 : depths.constLast();
    summary.feedXY = job.material.feedXY;
    summary.plungeFeed = job.material.plungeFeed;
    summary.spindleSpeed = job.material.spindleSpeed;
    summary.estimatedPathLength = geometry.xyLengthPerPass * summary.depthPassCount;

    for (const double depth : depths) {
        summary.estimatedPathLength += std::abs(job.approachZ - depth);
    }

    return summary;
}

GenerationResult GCodeGenerator::generate(const SurfacingJob &job) const
{
    GenerationResult result;
    const QStringList errors = validate(job);
    if (!errors.isEmpty()) {
        result.error = errors.join(QLatin1Char('\n'));
        return result;
    }

    result.geometry = buildGeometry(job);
    result.summary = summarize(job, result.geometry);

    Mach3PostProcessor postProcessor;
    result.gcode = postProcessor.generate(job, result.geometry, buildDepths(job));
    result.success = true;
    return result;
}
