#pragma once

#include "SurfacingJob.h"

#include <QStringList>

class GCodeGenerator
{
public:
    [[nodiscard]] QStringList validate(const SurfacingJob &job) const;
    [[nodiscard]] ToolpathGeometry buildGeometry(const SurfacingJob &job) const;
    [[nodiscard]] QVector<double> buildDepths(const SurfacingJob &job) const;
    [[nodiscard]] JobSummary summarize(const SurfacingJob &job,
                                       const ToolpathGeometry &geometry) const;
    [[nodiscard]] GenerationResult generate(const SurfacingJob &job) const;
};

