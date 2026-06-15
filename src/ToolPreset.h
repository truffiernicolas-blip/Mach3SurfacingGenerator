#pragma once

#include <QString>

struct ToolPreset
{
    QString name;
    int toolNumber = 1;
    double diameter = 20.0;
    int fluteCount = 2;
    double overlapPercent = 30.0;
    double recommendedSafeZ = 5.0;

    [[nodiscard]] double stepover() const
    {
        return diameter * (1.0 - overlapPercent / 100.0);
    }
};
