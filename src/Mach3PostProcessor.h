#pragma once

#include "SurfacingJob.h"

class Mach3PostProcessor
{
public:
    [[nodiscard]] QString generate(const SurfacingJob &job,
                                   const ToolpathGeometry &geometry,
                                   const QVector<double> &depths) const;

private:
    [[nodiscard]] static QString number(double value);
    [[nodiscard]] static QString commentText(QString value);
};

